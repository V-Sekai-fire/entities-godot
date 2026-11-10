// Rung A: picoquic driven by Flow's Net2 over IUDPSocket. Client and server share one process and one loop.
// Flow owns the sockets and the clock; picoquic is a state machine fed by feed() and drained by pull().
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "flow/flow.h"
#include "flow/serialize.h" // IConnection.h uses SendBuffer and is not self-contained
#include "flow/network.h"
#include "flow/IConnection.h"
#include "flow/IUDPSocket.h"
#include "flow/Platform.h"
#include "flow/TLSConfig.actor.h"
#include "flow/genericactors.actor.h"

#include "QuicDriver.h"

#include "flow/actorcompiler.h" // This must be the last #include.

struct QDatagram {
	Standalone<StringRef> data;
	NetworkAddress from;
};

static QDatagram makeDatagram(const uint8_t* p, int n, NetworkAddress from) {
	QDatagram d;
	d.data = Standalone<StringRef>(StringRef(p, n));
	d.from = from;
	return d;
}

static QuicAddr toQuicAddr(const NetworkAddress& a) {
	QuicAddr q;
	q.ip = a.ip.toV4();
	q.port = a.port;
	return q;
}

static NetworkAddress toNetworkAddress(QuicAddr q) {
	return NetworkAddress(IPAddress(q.ip), q.port, true, false);
}

// Socket-level counters, to tell a packet lost between the sockets from one picoquic dropped after feed().
struct IoCount {
	uint64_t sendDone = 0, sendBytes = 0, recvDone = 0, recvBytes = 0, nonV4 = 0;
};
static IoCount g_io[2]; // 0 = server socket, 1 = client socket

static void fireTrigger(void* p) {
	static_cast<AsyncTrigger*>(p)->trigger();
}

ACTOR Future<Void> quicRecvLoop(Reference<IUDPSocket> sock, PromiseStream<QDatagram> out, int which) {
	state std::vector<uint8_t> buf(2048);
	state NetworkAddress sender;
	loop {
		int n = wait(sock->receiveFrom(buf.data(), buf.data() + buf.size(), &sender));
		g_io[which].recvDone++;
		g_io[which].recvBytes += n;
		out.send(makeDatagram(buf.data(), n, sender));
	}
}

ACTOR Future<Void> quicDriver(Reference<IUDPSocket> sock,
                              QuicEndpoint* ep,
                              FutureStream<QDatagram> in,
                              AsyncTrigger* wake,
                              int which) {
	state QuicAddr local = toQuicAddr(sock->localAddress());
	state QuicOut out;
	state int64_t us = 0;
	loop {
		while (ep->pull(out)) {
			int sent =
			    wait(sock->sendTo(out.bytes.data(), out.bytes.data() + out.bytes.size(), toNetworkAddress(out.to)));
			g_io[which].sendDone++;
			g_io[which].sendBytes += sent;
		}
		us = ep->wakeDelayUs();
		if (us < 0)
			us = 0;
		choose {
			when(QDatagram d = waitNext(in)) {
				// Only IPv4 is handled. Count anything else instead of throwing in toV4() or dropping it silently.
				if (d.from.ip.isV4())
					ep->feed(d.data.begin(), d.data.size(), toQuicAddr(d.from), local);
				else
					g_io[which].nonV4++;
			}
			when(wait(delay(us / 1e6))) {}
			when(wait(wake->onTrigger())) {}
		}
	}
}

ACTOR Future<Void> quicScenario(QuicRunCfg cfg, QuicResult* res) {
	state QuicEndpoint server;
	state QuicEndpoint client;
	state AsyncTrigger serverWake;
	state AsyncTrigger clientWake;
	state PromiseStream<QDatagram> serverIn;
	state PromiseStream<QDatagram> clientIn;
	state Reference<IUDPSocket> ssock = wait(INetworkConnections::net()->createUDPSocket(false));
	state Reference<IUDPSocket> csock = wait(INetworkConnections::net()->createUDPSocket(false));
	state std::vector<uint8_t> payload(cfg.size, 0xAA);
	state int i = 0;
	state int k = 0;

	ssock->bind(NetworkAddress::parse("127.0.0.1:0"));
	csock->bind(NetworkAddress::parse("127.0.0.1:0"));
	if (!(cfg.wt ? server.initServerWt(cfg.cert.c_str(), cfg.key.c_str())
	             : server.initServer(cfg.cert.c_str(), cfg.key.c_str(), "flow-echo"))) {
		res->setupFailed = true;
		res->setupError = "server init failed (certificate or key not loaded)";
		return Void();
	}
	if (!(cfg.wt ? client.initClientWt(cfg.root.c_str()) : client.initClient(cfg.root.c_str(), "flow-echo"))) {
		res->setupFailed = true;
		res->setupError = "client init failed";
		return Void();
	}
	server.setWake(&fireTrigger, &serverWake);
	client.setWake(&fireTrigger, &clientWake);
	if (!(cfg.wt ? client.connectWt(toQuicAddr(ssock->localAddress()), cfg.sni.c_str(), cfg.badpath ? "/nope" : "/echo")
	             : client.connect(toQuicAddr(ssock->localAddress()), cfg.sni.c_str()))) {
		res->setupFailed = true;
		res->setupError = "client connect refused: " + client.connectError;
		return Void();
	}

	state Future<Void> sr = quicRecvLoop(ssock, serverIn, 0);
	state Future<Void> cr = quicRecvLoop(csock, clientIn, 1);
	state Future<Void> sd = quicDriver(ssock, &server, serverIn.getFuture(), &serverWake, 0);
	state Future<Void> cd = quicDriver(csock, &client, clientIn.getFuture(), &clientWake, 1);
	clientWake.trigger();

	// Wait for the handshake to finish or fail (10 s at most).
	loop {
		wait(delay(0.005));
		client.snapshotState();
		if (client.stats.ready || client.stats.closed || client.stats.refused > 0 || ++i > 2000)
			break;
	}
	if (client.stats.ready) {
		wait(delay(0.5)); // let path MTU discovery finish before sending full-size datagrams
		for (k = 0; k < cfg.count; k++) {
			payload[0] = k & 0xff;
			payload[1] = (k >> 8) & 0xff;
			client.queueDatagram(payload.data(), payload.size());
			wait(delay(1.0 / 64)); // 64 Hz
		}
	}
	wait(delay(0.5));
	client.snapshotState();
	server.snapshotState();
	res->client = client.stats;
	res->server = server.stats;
	return Void();
}

ACTOR Future<Void> quicStopAfter(Future<Void> f) {
	choose {
		when(wait(ready(f))) {}
		when(wait(delay(60.0))) {}
	}
	g_network->stop();
	return Void();
}

int main(int argc, char** argv) {
	if (argc < 6) {
		std::fprintf(stderr, "usage: flow_quic_echo CERT KEY ROOT SNI positive|control|badpath [COUNT] [BYTES] [wt]\n");
		return 2;
	}
	QuicRunCfg cfg;
	cfg.cert = argv[1];
	cfg.key = argv[2];
	cfg.root = argv[3];
	cfg.sni = argv[4];
	cfg.control = std::strcmp(argv[5], "control") == 0;
	cfg.badpath = std::strcmp(argv[5], "badpath") == 0;
	cfg.wt = argc > 8 && std::strcmp(argv[8], "wt") == 0;
	if (argc > 6)
		cfg.count = std::atoi(argv[6]);
	if (argc > 7)
		cfg.size = std::atoi(argv[7]);
	if (std::getenv("FLOWQUIC_DEBUG"))
		quicDebugToStderr();
	platformInit();
	g_network = newNet2(TLSConfig(), false, true);
	QuicResult res;
	Future<Void> f = quicStopAfter(quicScenario(cfg, &res));
	g_network->run();
	std::printf("sockets: client sent %llu (%llu bytes) recv %llu (%llu bytes); server sent %llu (%llu bytes) recv "
	            "%llu (%llu bytes); non-IPv4 senders dropped: client %llu server %llu\n",
	            (unsigned long long)g_io[1].sendDone,
	            (unsigned long long)g_io[1].sendBytes,
	            (unsigned long long)g_io[1].recvDone,
	            (unsigned long long)g_io[1].recvBytes,
	            (unsigned long long)g_io[0].sendDone,
	            (unsigned long long)g_io[0].sendBytes,
	            (unsigned long long)g_io[0].recvDone,
	            (unsigned long long)g_io[0].recvBytes,
	            (unsigned long long)g_io[1].nonV4,
	            (unsigned long long)g_io[0].nonV4);
	return quicJudge(cfg, res);
}
