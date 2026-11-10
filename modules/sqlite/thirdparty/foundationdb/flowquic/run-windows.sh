#!/bin/bash
# Runs flow_quic_echo on the Windows runner, three times per case, none stopping the next.
# A run passes only when the exit status is 0 AND the output holds a PASS line, so a binary that
# exits early or prints nothing cannot pass. A failed run is repeated once with FLOWQUIC_DEBUG=1.
# Run from the repository root.
set +e
EXE=build/bin/Release/flow_quic_echo.exe
PQ=build/_deps/picoquic_src-src/certs
SNI=test.example.com

[ -f "$EXE" ] || { echo "::error::$EXE is missing"; exit 1; }
for f in cert.pem key.pem test-ca.crt badcert.pem; do
  [ -f "$PQ/$f" ] || { echo "::error::$PQ/$f is missing"; exit 1; }
done

total_fail=0

# case NAME ROOT MODE EXTRA...   (EXTRA is COUNT BYTES [wt])
run_case() {
  name=$1; root=$2; mode=$3; shift 3
  pass=0
  for i in 1 2 3; do
    log="flowquic-run-$name-$i.log"
    timeout 60 "$EXE" "$PQ/cert.pem" "$PQ/key.pem" "$PQ/$root" "$SNI" "$mode" "$@" > "$log" 2>&1
    rc=$?
    if [ $rc -eq 0 ] && grep -q '^PASS' "$log"; then
      pass=$((pass + 1))
      echo "$name run $i: PASS"
    else
      echo "$name run $i: FAIL (exit $rc)"
      FLOWQUIC_DEBUG=1 timeout 60 "$EXE" "$PQ/cert.pem" "$PQ/key.pem" "$PQ/$root" "$SNI" "$mode" "$@" \
        > "flowquic-run-$name-$i-debug.log" 2>&1
    fi
    grep -E '^(PASS|FAIL)|^sockets|^packets|TLS alert|^control' "$log" | cut -c1-240
  done
  echo "CASE $name: $pass of 3 passed"
  [ "$pass" -eq 3 ] || total_fail=1
}

run_case quic-positive test-ca.crt positive 192 1200
run_case quic-control  badcert.pem control  192 1200
run_case wt-positive   test-ca.crt positive 192 1200 wt
run_case wt-control    badcert.pem control  192 1200 wt
run_case wt-badpath    test-ca.crt badpath  192 1200 wt

exit $total_fail
