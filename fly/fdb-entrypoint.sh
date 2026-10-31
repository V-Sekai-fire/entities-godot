#!/bin/sh
# Form a FoundationDB cluster from whatever machines Fly has started.
#
# The problem this solves. A coordinator list has to be written into a cluster file before
# any process starts, and on Fly a machine's address does not exist until the machine does.
# There is no address to write at build time and no machine to ask at deploy time, so the
# list is discovered at boot instead: every machine resolves the app's internal DNS, which
# returns an AAAA record for each machine in the app, and waits until it sees the expected
# count.
#
# Why waiting for a *count* rather than starting with whoever answers. A cluster file naming
# one coordinator is a valid cluster file. Three machines that each wrote their own would
# form three single-node clusters that never merge, and the deploy would look successful --
# `fly status` reports three healthy machines and every one of them serves reads. That is the
# failure this waits to avoid, and it is why WEFT_FDB_MACHINES is required rather than
# defaulted: a default would silently be wrong on the day someone scales to five.
set -eu

: "${FLY_APP_NAME:?not running on Fly}"
: "${WEFT_FDB_MACHINES:?set WEFT_FDB_MACHINES to the number of machines in this app}"
CLUSTER_DESC=${WEFT_FDB_CLUSTER_DESC:-weft}
CLUSTER_ID=${WEFT_FDB_CLUSTER_ID:?set WEFT_FDB_CLUSTER_ID}
DATA=${WEFT_FDB_DATA:-/data}
PORT=${WEFT_FDB_PORT:-4500}
PROCS=${WEFT_FDB_PROCS:-2}
REDUNDANCY=${WEFT_FDB_REDUNDANCY:-double}
STORAGE=${WEFT_FDB_STORAGE:-ssd-2}

log() { echo "[entrypoint] $*"; }

# This machine's own 6PN address. Fly publishes it as `fly-local-6pn`, which is the only
# name guaranteed to be this machine and not a sibling.
self=$(getent hosts fly-local-6pn | awk '{print $1; exit}')
[ -n "$self" ] || { echo "cannot resolve fly-local-6pn" >&2; exit 1; }
log "self $self  machine ${FLY_MACHINE_ID:-?}  region ${FLY_REGION:-?}"

# Wait for every machine to have an address. 6PN DNS is eventually consistent while a deploy
# rolls, so this polls rather than reading once.
peers=""
waited=0
while [ "$waited" -lt "${WEFT_FDB_DNS_TIMEOUT:-180}" ]; do
	peers=$(getent ahostsv6 "${FLY_APP_NAME}.internal" 2>/dev/null \
		| awk '{print $1}' | sort -u | grep -v '^$' || true)
	n=$(printf '%s\n' "$peers" | grep -c . || true)
	if [ "$n" -ge "$WEFT_FDB_MACHINES" ]; then
		log "found $n of $WEFT_FDB_MACHINES machines"
		break
	fi
	log "waiting for machines: $n of $WEFT_FDB_MACHINES"
	sleep 3
	waited=$((waited + 3))
done
n=$(printf '%s\n' "$peers" | grep -c . || true)
if [ "$n" -lt "$WEFT_FDB_MACHINES" ]; then
	echo "only $n of $WEFT_FDB_MACHINES machines resolved after ${waited}s" >&2
	echo "Refusing to start: a partial coordinator list forms a cluster that looks healthy" >&2
	echo "and is not the cluster that was asked for." >&2
	exit 1
fi

# The coordinator list. Sorted, so every machine writes byte-identical text: FoundationDB
# compares cluster files between processes, and two orderings of the same three addresses
# are two different files to it.
#
# Exactly WEFT_FDB_MACHINES coordinators, taken in sorted order, so scaling past three
# still yields an odd, agreed-upon set rather than everyone who happened to answer.
# A TLS cluster's coordinator addresses carry a `:tls` suffix, and it is part of the address
# rather than decoration: a process reading an address without it speaks plaintext to a peer
# that will not, and the connection fails with no indication that TLS was the reason.
sfx=""
# Keyed on the CA rather than the certificate, because the certificate is per-machine and is
# not chosen until further down -- this runs first, so it would always have seen an empty
# value and written a plaintext coordinator list while each process advertised `:tls`.
# FoundationDB reports that as "TLS state of public address does not match in coordinator
# list", which names the symptom and not the ordering.
[ -n "${FDB_TLS_CA_B64:-}" ] && sfx=":tls"
coords=$(printf '%s\n' "$peers" | sort | head -n "$WEFT_FDB_MACHINES" \
	| sed "s|.*|[&]:$PORT$sfx|" | paste -sd, -)
log "coordinators $coords"

mkdir -p /etc/foundationdb "$DATA" /var/log/foundationdb

# Discard the data directory and start empty.
#
# This exists because there is no in-place path from a plaintext cluster to a TLS one.
# A coordinator's address is part of its identity, `:tls` is part of the address, and the
# coordinated state on disk was written under the old one -- so the processes come up, speak
# TLS to each other correctly, and never reach a quorum, because the quorum they are looking
# for is recorded at addresses that no longer exist. `status` reports "Could not communicate
# with a quorum of coordination servers" and every other signal looks healthy.
#
# So enabling TLS on a running cluster means recreating it. That is a real cost and it is
# written here rather than rediscovered: decide TLS before there is data worth keeping, or
# plan a backup and restore around the change.
if [ "${WEFT_FDB_RESET:-0}" = 1 ]; then
	log "WEFT_FDB_RESET=1: discarding everything under $DATA"
	rm -rf "${DATA:?}"/* 2>/dev/null || true
	log "$DATA now holds $(ls -A "$DATA" 2>/dev/null | wc -l) entries"
fi

# TLS, if the material was given. One wildcard certificate is presented by every process and
# verified by every peer against the same subject, which is what makes one certificate enough
# for a whole cluster: FoundationDB pins on the certificate's subject rather than matching a
# hostname against the address it connected to. On 6PN there is no hostname to match anyway,
# since peers reach each other at IPv6 literals.
#
# The material arrives base64-encoded. A PEM is multi-line, and a multi-line value survives
# neither a secrets store nor a shell round-trip reliably enough to be trusted with the thing
# standing between this database and anyone who can route to it.
tls=0
# Each machine carries its own certificate, named by its whole Fly machine ID.
#
# It was the NIC first, and a NIC does not survive `fly machine update`: the same three
# machines came back as ee13/30cb where they had been 3678/fbc7, so every certificate named
# hardware that no longer existed and all three refused to start. The machine ID did not move
# across any of those updates -- it is the identity Fly keeps, and the volume is attached to
# it -- so it is the thing worth naming.
#
# The whole ID rather than a suffix of it. Four hex characters is 65,536 values, which puts
# two machines in collision with probability around n^2/131072: nothing at three, about 7% at
# a hundred, even odds near three hundred. The full ID is unique by construction, so there is
# no fleet size at which this quietly stops holding and no birthday estimate for a later
# reader to check. The cost is a longer name, which nothing here reads aloud.
#
# Fly secrets are app-wide rather than per-machine, so every pair is present on every machine
# and each takes the one addressed to it. A machine that finds no pair for itself stops,
# because the alternative is silently adopting another machine's identity, and peer
# verification cannot tell the difference.
mid=$(printf '%s' "${FLY_MACHINE_ID:-}" | tr 'A-Z' 'a-z')
if [ -n "${mid:-}" ] && [ -z "${FDB_TLS_CERT_B64:-}" ]; then
	eval "FDB_TLS_CERT_B64=\${FDB_TLS_CERT_${mid}_B64:-}"
	eval "FDB_TLS_KEY_B64=\${FDB_TLS_KEY_${mid}_B64:-}"
	if [ -n "${FDB_TLS_CERT_B64:-}" ]; then
		log "TLS identity fdb-$mid"
	elif [ -n "${FDB_TLS_CA_B64:-}" ]; then
		echo "a CA was given but no certificate for this machine ($mid)" >&2
		echo "Expected FDB_TLS_CERT_${mid}_B64. Refusing to start rather than borrow another" >&2
		echo "machine's identity, which would make peer verification meaningless." >&2
		exit 1
	fi
fi

if [ -n "${FDB_TLS_CERT_B64:-}" ]; then
	[ -n "${FDB_TLS_KEY_B64:-}" ] || { echo "FDB_TLS_CERT_B64 without FDB_TLS_KEY_B64" >&2; exit 1; }
	[ -n "${FDB_TLS_CA_B64:-}" ]  || { echo "FDB_TLS_CERT_B64 without FDB_TLS_CA_B64" >&2; exit 1; }
	mkdir -p /etc/foundationdb/tls
	printf '%s' "$FDB_TLS_CERT_B64" | base64 -d > /etc/foundationdb/tls/cert.pem
	printf '%s' "$FDB_TLS_KEY_B64"  | base64 -d > /etc/foundationdb/tls/key.pem
	printf '%s' "$FDB_TLS_CA_B64"   | base64 -d > /etc/foundationdb/tls/ca.pem
	# Complete the chain to a self-signed root, and why this is not optional.
	#
	# Let's Encrypt hands out an issuer chain that ends at a *cross-signed* root -- here
	# `ISRG Root X2` as signed by `ISRG Root X1` -- and not at a self-signed one. OpenSSL's
	# `verify` accepts that, because `-CAfile` treats every certificate in the file as a
	# trust anchor, so a check built on it reports OK. FoundationDB does not: it wants a path
	# to a self-signed root, and without one every handshake fails with
	# `TLSPolicyFailure Reason="preverification"`.
	#
	# The failure mode is the expensive part. The processes start, report healthy, speak TLS
	# to each other perfectly, and never form a quorum -- `status` says only "Could not
	# communicate with a quorum of coordination servers", which reads like a networking or
	# coordinator problem and not like a certificate one.
	#
	# Only the roots that actually anchor this chain are appended, rather than the system
	# bundle. Appending the bundle would work and would quietly widen the trusted issuer set
	# from one CA to every public CA, leaving `S.CN` as the only thing between this cluster
	# and anyone who can get a certificate for the name.
	# The CA file is used exactly as given. Nothing is appended and nothing is truncated.
	#
	# An earlier version completed the chain from the system trust store, written when the
	# certificate came from a public CA whose chain ends at a cross-signed root. Against a
	# private CA that would have thrown away the only certificate that matters and trusted the
	# public roots instead: the cluster would accept anything a public CA had ever issued for
	# the pinned name, and reject the certificate it was actually given.
	#
	# A private root needs none of it. It is self-signed, so it is already an anchor -- which
	# is the property Let's Encrypt's cross-signed chain could not provide and the reason
	# every handshake failed at `preverification`.

	# A self-signed certificate is one whose subject is its own issuer. If none survived the
	# step above, the chain has no anchor and the handshake will fail on every peer -- so it
	# fails here instead, where the message can say so.
	anchors=0
	csplit -sz -f /tmp/ca- -b '%02d.pem' /etc/foundationdb/tls/ca.pem '/BEGIN CERTIFICATE/' '{*}' 2>/dev/null || true
	for c in /tmp/ca-*.pem; do
		[ -f "$c" ] || continue
		sub=$(openssl x509 -in "$c" -noout -subject 2>/dev/null | sed 's/^subject=//')
		iss=$(openssl x509 -in "$c" -noout -issuer 2>/dev/null | sed 's/^issuer=//')
		[ -n "$sub" ] && [ "$sub" = "$iss" ] && anchors=$((anchors + 1))
	done
	rm -f /tmp/ca-*.pem
	log "TLS ca.pem: $anchors self-signed anchor(s)"
	if [ "$anchors" = 0 ]; then
		echo "the CA file contains no self-signed root" >&2
		echo "FoundationDB needs a path to a self-signed anchor. `openssl verify -CAfile` will" >&2
		echo "accept a cross-signed chain and FoundationDB will not, so that check does not" >&2
		echo "cover this. Every peer handshake would fail with Reason=preverification." >&2
		exit 1
	fi

	chmod 0600 /etc/foundationdb/tls/key.pem
	chmod 0644 /etc/foundationdb/tls/cert.pem /etc/foundationdb/tls/ca.pem

	# The key must match the certificate. They are separate secrets, so nothing else checks
	# they were rotated together -- and a mismatched pair fails at handshake time, on a peer,
	# as a connection error rather than as a configuration error. Compared here, where the
	# message can say what is actually wrong.
	cpub=$(openssl x509 -in /etc/foundationdb/tls/cert.pem -noout -pubkey 2>/dev/null | openssl dgst -sha256 | awk '{print $NF}')
	kpub=$(openssl pkey -in /etc/foundationdb/tls/key.pem -pubout 2>/dev/null | openssl dgst -sha256 | awk '{print $NF}')
	if [ -z "$cpub" ] || [ "$cpub" != "$kpub" ]; then
		echo "the TLS key does not match the certificate" >&2
		echo "  cert public key sha256 ${cpub:-unreadable}" >&2
		echo "  key  public key sha256 ${kpub:-unreadable}" >&2
		exit 1
	fi

	subject=$(openssl x509 -in /etc/foundationdb/tls/cert.pem -noout -subject 2>/dev/null | sed 's/^subject=//')
	notafter=$(openssl x509 -in /etc/foundationdb/tls/cert.pem -noout -enddate | sed 's/notAfter=//')
	VERIFY=${WEFT_FDB_TLS_VERIFY:?set WEFT_FDB_TLS_VERIFY when TLS material is given}
	log "TLS on: subject [$subject] expires $notafter"
	log "TLS verify_peers: $VERIFY"
	tls=1

	export FDB_TLS_CERTIFICATE_FILE=/etc/foundationdb/tls/cert.pem
	export FDB_TLS_KEY_FILE=/etc/foundationdb/tls/key.pem
	export FDB_TLS_CA_FILE=/etc/foundationdb/tls/ca.pem
	export FDB_TLS_VERIFY_PEERS="$VERIFY"
else
	log "TLS off: no FDB_TLS_CERT_B64. Every byte between these processes is in the clear."
fi
echo "${CLUSTER_DESC}:${CLUSTER_ID}@${coords}" > /etc/foundationdb/fdb.cluster
chmod 0644 /etc/foundationdb/fdb.cluster
log "cluster file: $(cat /etc/foundationdb/fdb.cluster)"

# One fdbserver per process slot, each on its own port and data directory.
#
# `locality_zoneid` is the machine, not the region. Fault tolerance is counted in zones, so
# telling FoundationDB that three machines are one zone would let it place all replicas of a
# key on one machine and still report itself healthy -- the cluster would claim tolerance it
# does not have, which is the exact failure `qa/consensus.sh` step 3 exists to catch.
# Blob store credentials for backup, if the bucket is wired up.
#
# The secret is deliberately NOT in the backup URL. `fdbbackup` accepts
# `blobstore://<key>:<secret>@host/...`, and every use of that form puts the secret in a
# command line, in shell history, and in `ps` output for every process on the machine. The
# credentials file exists for this: the URL names `<key>@host` and the secret is resolved at
# connect time from a file only root can read.
#
# The account key is `<access key id>@<host>` and it must match the URL exactly, or the
# lookup fails and the error names the account rather than the file.
blob_creds=/etc/foundationdb/blob-credentials.json
backup=0
if [ -n "${AWS_ACCESS_KEY_ID:-}" ] && [ -n "${AWS_SECRET_ACCESS_KEY:-}" ] && [ -n "${AWS_ENDPOINT_URL_S3:-}" ]; then
	blob_host=$(printf '%s' "$AWS_ENDPOINT_URL_S3" | sed -e 's|^https\?://||' -e 's|/.*$||')
	umask 077
	printf '{"accounts":{"%s@%s":{"secret":"%s"}}}
' 		"$AWS_ACCESS_KEY_ID" "$blob_host" "$AWS_SECRET_ACCESS_KEY" > "$blob_creds"
	umask 022
	chmod 0600 "$blob_creds"
	export FDB_BLOB_CREDENTIALS=$blob_creds
	backup=1

	# FoundationDB never dials the blob store directly. Its TLS client sends no
	# SNI, and Tigris's edge closes any handshake without one -- measured as
	# N2_ConnectHandshakeError "stream truncated" on the A and the AAAA record
	# alike, while openssl with -servername succeeds from the same machine. An
	# stunnel client on loopback adds the SNI and verifies the public chain.
	#
	# The endpoint's NAME must survive the hop: Tigris routes the bucket from the
	# Host header, and a request arriving as Host: 127.0.0.1 was answered with
	# <BucketName>127.0.0.1</BucketName>, the path read as a key. So the IP is
	# resolved first, stunnel dials the literal, and /etc/hosts then points the
	# name at loopback -- FoundationDB keeps signing and sending the real
	# hostname while its bytes go one syscall away. The IP is pinned until the
	# next machine start; a rotation surfaces as checkHost refusing the new
	# certificate, not as silent data to a stranger.
	blob_ip=$(getent ahostsv4 "$blob_host" | head -1 | cut -d' ' -f1)
	[ -n "$blob_ip" ] || { echo "cannot resolve $blob_host" >&2; exit 1; }
	cat > /etc/foundationdb/stunnel-blob.conf <<-EOF
		foreground = no
		output = /var/log/foundationdb/stunnel-blob.log
		pid = /var/run/stunnel-blob.pid
		[blob]
		client = yes
		accept = 127.0.0.1:8443
		connect = ${blob_ip}:443
		sni = ${blob_host}
		CAfile = /etc/ssl/certs/ca-certificates.crt
		verifyChain = yes
		checkHost = ${blob_host}
	EOF
	stunnel4 /etc/foundationdb/stunnel-blob.conf
	echo "127.0.0.1 $blob_host" >> /etc/hosts
	log "stunnel 127.0.0.1:8443 -> $blob_ip:443 adds the SNI FoundationDB does not send"

	# The URL is written here rather than left to whoever runs fdbbackup, because two of
	# its parts are load bearing and neither announces itself when wrong.
	#
	# The port is not decoration. Without it FoundationDB substitutes the service *name*
	# "https" and resolves it through /etc/services, which fails as lookup_failed and
	# reports itself as a DNS fault. netbase now ships that file, so this is the second
	# guard rather than the only one.
	#
	# The region is explicit because guessRegionFromDomain does not recognise this
	# endpoint. A guessed region signs the request wrongly and comes back as an auth
	# error, which reads as bad credentials rather than a bad URL.
	#
	# Mode 0600 and beside the credentials: the access key is in the URL, and it is the
	# same key that file already holds.
	# There used to be a fatal A-record assertion here, because FoundationDB's
	# pickOneAddress prefers IPv6 with no fallback and no knob to require IPv4.
	# stunnel dials the endpoint now and falls back across families like any
	# ordinary client, so the address family stopped being a precondition.

	backup_url=/etc/foundationdb/backup-url
	umask 077
	# sc=0 on loopback: the TLS starts at stunnel, one syscall away. The v4 knob in
	# the printed command is not optional either -- FoundationDB signs SigV2 by
	# default and Tigris (like versitygw, where this was measured) refuses
	# anything but SigV4.
	printf 'blobstore://%s@%s:8443/%s?bucket=%s&region=%s&sc=0\n' \
		"$AWS_ACCESS_KEY_ID" "$blob_host" "${WEFT_BACKUP_NAME:-weft}" \
		"${BUCKET_NAME:-unset}" "${AWS_REGION:-auto}" > "$backup_url"
	umask 022
	chmod 0600 "$backup_url"

	log "blob store $blob_host via loopback 8443, bucket ${BUCKET_NAME:-unset} region ${AWS_REGION:-auto}"
	log "backup url in $backup_url -- fdbbackup start -z -d \"\$(cat $backup_url)\" \\"
	log "  --blob-credentials $blob_creds --knob_http_request_aws_v4_header=true"

	# Freshness of the default tag, published as /health for fdb.toml's machine check;
	# the loop refuses to arm unless its own controls fire (see backup-fresh.sh).
	/usr/local/bin/backup-fresh.sh >> /var/log/foundationdb/backup-fresh.log 2>&1 &
	log "backup freshness check on :8081/health, max ${WEFT_BACKUP_MAX_BEHIND:-3600}s behind"

	# There used to be a second trust bundle here, cluster root plus public roots,
	# because the agent's one TLS policy covered both peers and the blob store.
	# The stunnel hop retires it: the agent's blob traffic is loopback plaintext,
	# the public-root verification happens in stunnel, and the agent's TLS policy
	# goes back to covering exactly the cluster. A trust store that admits every
	# public CA never touches a FoundationDB process again.
else
	log "no blob store: backup is not configured on this machine"
fi

# The DR destination. R2 Infrequent Access, reached through a second stunnel
# hop for the same SNI reason. Guarded by its own env: a machine without the
# R2 credentials never writes /dr, so its check is critical rather than
# passing on nothing, and the negative control fires on the machine check.
blob_creds_r2=/etc/foundationdb/blob-credentials-r2.json
backup_r2=0
if [ -n "${R2_ACCESS_KEY_ID:-}" ] && [ -n "${R2_SECRET_ACCESS_KEY:-}" ] && [ -n "${R2_ENDPOINT_URL:-}" ]; then
	blob_host_r2=$(printf '%s' "$R2_ENDPOINT_URL" | sed -e 's|^https\?://||' -e 's|/.*$||')
	umask 077
	printf '{"accounts":{"%s@%s":{"secret":"%s"}}}\n' \
		"$R2_ACCESS_KEY_ID" "$blob_host_r2" "$R2_SECRET_ACCESS_KEY" > "$blob_creds_r2"
	umask 022
	chmod 0600 "$blob_creds_r2"
	export FDB_BLOB_CREDENTIALS="$blob_creds:$blob_creds_r2"
	backup_r2=1

	blob_ip_r2=$(getent ahostsv4 "$blob_host_r2" | head -1 | cut -d' ' -f1)
	[ -n "$blob_ip_r2" ] || { echo "cannot resolve $blob_host_r2" >&2; exit 1; }
	cat > /etc/foundationdb/stunnel-blob-r2.conf <<-EOF
		foreground = no
		output = /var/log/foundationdb/stunnel-blob-r2.log
		pid = /var/run/stunnel-blob-r2.pid
		[blob-r2]
		client = yes
		accept = 127.0.0.1:8444
		connect = ${blob_ip_r2}:443
		sni = ${blob_host_r2}
		CAfile = /etc/ssl/certs/ca-certificates.crt
		verifyChain = yes
		checkHost = ${blob_host_r2}
	EOF
	stunnel4 /etc/foundationdb/stunnel-blob-r2.conf
	echo "127.0.0.1 $blob_host_r2" >> /etc/hosts
	log "R2 stunnel 127.0.0.1:8444 -> $blob_ip_r2:443"

	backup_url_r2=/etc/foundationdb/backup-url-r2
	umask 077
	printf 'blobstore://%s@%s:8444/%s?bucket=%s&region=%s&sc=0\n' \
		"$R2_ACCESS_KEY_ID" "$blob_host_r2" "${WEFT_BACKUP_NAME:-weft}" \
		"${R2_BUCKET:-weftspun-fdb-dr}" "${R2_REGION:-auto}" > "$backup_url_r2"
	umask 022
	chmod 0600 "$backup_url_r2"

	log "R2 backup url in $backup_url_r2 -- fdbbackup start -t dr -d \"\$(cat $backup_url_r2)\""

	# Started here rather than beside the default tag's loop: R2 is configured
	# without Tigris, and the DR check must not depend on a store it never reads.
	/usr/local/bin/backup-fresh.sh --tag dr >> /var/log/foundationdb/backup-fresh-dr.log 2>&1 &
	log "DR backup freshness check on :8081/dr, tag dr"
else
	log "no R2 blob store: DR tag is not configured on this machine"
fi

# Each process's share of 80 per cent of the VM, a quarter of it page cache; fly.toml may override.
total_kib=$(awk '/^MemTotal:/ {print $2}' /proc/meminfo)
mem_mib=${WEFT_FDB_MEMORY_MIB:-$((total_kib * 8 / 10 / 1024 / PROCS))}
cache_mib=${WEFT_FDB_CACHE_MIB:-$((mem_mib / 4))}
log "memory ${mem_mib}MiB and cache_memory ${cache_mib}MiB per process, from MemTotal ${total_kib} KiB over $PROCS processes"

conf=/etc/foundationdb/foundationdb.conf
{
	echo "[fdbmonitor]"
	echo "user = root"
	echo "group = root"
	echo ""
	echo "[general]"
	echo "restart_delay = 10"
	echo "cluster_file = /etc/foundationdb/fdb.cluster"
	echo ""
	echo "[fdbserver]"
	echo "command = /usr/sbin/fdbserver"
	echo "datadir = $DATA/\$ID"
	echo "logdir = /var/log/foundationdb"
	if [ "$tls" = 1 ]; then
		echo "public_address = [$self]:\$ID:tls"
		echo "tls_certificate_file = /etc/foundationdb/tls/cert.pem"
		echo "tls_key_file = /etc/foundationdb/tls/key.pem"
		echo "tls_ca_file = /etc/foundationdb/tls/ca.pem"
		echo "tls_verify_peers = $VERIFY"
	else
		echo "public_address = [$self]:\$ID"
	fi
	echo "listen_address = public"
	echo "locality_zoneid = ${FLY_MACHINE_ID:-$self}"
	echo "locality_machineid = ${FLY_MACHINE_ID:-$self}"
	echo "locality_dcid = ${FLY_REGION:-unknown}"
	# FoundationDB's defaults (memory 8GiB, cache_memory 2GiB) assume a server, so on a small VM
	# the kernel kills a process before FoundationDB sheds anything. Size both from this VM.
	echo "memory = ${mem_mib}MiB"
	echo "cache_memory = ${cache_mib}MiB"
	i=0
	while [ "$i" -lt "$PROCS" ]; do
		echo ""
		echo "[fdbserver.$((PORT + i))]"
		i=$((i + 1))
	done

	# The backup agent. It is a *client* of the cluster, not a server, so under mutual TLS it
	# needs its own certificate, key, CA and verification rule exactly as fdbcli does. An
	# agent started without them connects to nothing and reports no error worth reading.
	#
	# One agent for each machine. Any number of agents pointed at the same database cooperate
	# on one backup, so this is throughput rather than redundancy, and a machine that is down
	# takes its agent with it either way.
	# Gate the agent block on EITHER destination being wired, not just Tigris.
	# An R2-only setup -- the DR path RFD 2143 describes -- otherwise spawns no
	# agent at all, and `fdbrestore` queues forever with no worker. RFD 2144
	# defect #6 caught this the hard way; the negative control is a machine
	# started with R2_* only, where `pgrep backup_agent` returns at least one pid.
	if [ "$backup" = 1 ] || [ "$backup_r2" = 1 ]; then
		# The credentials file the agent reads. When both destinations exist,
		# FoundationDB accepts multiple files separated by ':' and picks the
		# entry whose account key matches the URL at connect time.
		if [ "$backup" = 1 ] && [ "$backup_r2" = 1 ]; then
			creds="$blob_creds:$blob_creds_r2"
		elif [ "$backup" = 1 ]; then
			creds="$blob_creds"
		else
			creds="$blob_creds_r2"
		fi
		echo ""
		echo "[backup_agent]"
		echo "command = /usr/lib/foundationdb/backup_agent/backup_agent"
		echo "logdir = /var/log/foundationdb"
		echo "blob_credentials = $creds"
		# The blob store rides the loopback hop now, so the resolver preferences and the blob
		# trust additions this section used to carry are gone with the direct dial.
		# SigV4, because the endpoint behind the stunnel hop refuses SigV2, and
		# FoundationDB signs SigV2 unless told otherwise.
		echo "knob_http_request_aws_v4_header = true"
		if [ "$tls" = 1 ]; then
			echo "tls_certificate_file = /etc/foundationdb/tls/cert.pem"
			echo "tls_key_file = /etc/foundationdb/tls/key.pem"
			echo "tls_ca_file = /etc/foundationdb/tls/ca.pem"
			echo "tls_verify_peers = $VERIFY"
		fi
		echo ""
		echo "[backup_agent.1]"
		# A second agent for the DR tag. Backup agents cooperate on any tag the
		# database is running, so this is one more worker rather than a mirror.
		if [ "$backup" = 1 ] && [ "$backup_r2" = 1 ]; then
			echo ""
			echo "[backup_agent.2]"
		fi
	fi
} > "$conf"
log "$PROCS processes from port $PORT"

/usr/lib/foundationdb/fdbmonitor --conffile "$conf" --lockfile /var/run/fdbmonitor.pid &
monitor=$!
log "fdbmonitor pid $monitor"

# The roll gate, in the Kubernetes FDB operator's manner: Fly advances a
# rolling deploy only when every machine check passes, and this one fails
# until the data state is healthy and a zone can be lost without losing
# data. A restart therefore waits out its predecessor's re-replication
# instead of stalling shards whose replicas span both machines.
# One httpd serves both health files: /health (backup freshness, when the
# blob store is wired) and /cluster (the roll gate, always).
mkdir -p /run/backup-fresh
busybox httpd -p 8081 -h /run/backup-fresh
/usr/local/bin/cluster-health.sh >> /var/log/foundationdb/cluster-health.log 2>&1 &
log "cluster health gate on :8081/cluster"

# Whoever holds the lowest address configures the database, once. A deterministic choice
# rather than a lock, because `configure new` is idempotent in the way that matters: it
# fails on a database that already exists, and the failure is the desired outcome for the
# other two machines.
lowest=$(printf '%s\n' "$peers" | sort | head -1)
if [ "$self" = "$lowest" ]; then
	log "lowest address, so this machine configures the database and starts backup tags"
	waited=0
	while [ "$waited" -lt 120 ]; do
		if fdbcli --exec 'status minimal' 2>&1 | grep -q 'The database is available'; then
			log "database already available"
			break
		fi
		out=$(fdbcli --exec "configure new $REDUNDANCY $STORAGE" 2>&1 || true)
		log "configure: $out"
		case "$out" in
			*"Database created"*|*"already exists"*) break ;;
		esac
		sleep 5
		waited=$((waited + 5))
	done
	fdbcli --exec 'status minimal' 2>&1 | sed 's/^/[entrypoint] /' || true

	# Start the DR tag once. `fdbbackup start` refuses to overlay a running
	# tag ("already exists"), so gate on `fdbbackup status -t dr` reporting
	# no backup. The refusal case is the normal case on every restart after
	# the first; guarding it keeps the log meaningful when it does fire.
	if [ "$backup_r2" = 1 ]; then
		st=$(fdbcli --exec 'status minimal' 2>&1)
		case "$st" in
			*'database is available'*)
				out=$(fdbbackup status -t dr 2>&1 || true)
				case "$out" in
					*'No previous backups found'*|*'The previous backup on tag'*'terminated'*|*'has been submitted for termination'*)
						log "starting DR backup: fdbbackup start -t dr"
						fdbbackup start -t dr -z \
							-d "$(cat "$backup_url_r2")" \
							--blob-credentials "$blob_creds_r2" \
							--knob_http_request_aws_v4_header=true 2>&1 \
							| sed 's/^/[entrypoint] fdbbackup dr: /'
						;;
					*'is restorable'*|*'submitted'*)
						log "DR backup already running"
						;;
					*)
						log "DR backup status unclear, not starting: $out"
						;;
				esac
				;;
			*)
				log "DB not available yet for DR start"
				;;
		esac
	fi
fi

wait "$monitor"
