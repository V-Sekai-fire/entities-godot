#pragma once
// A thin, plain-C++ wrapper over picoquic. No Flow types and no picoquic types in this header, so the
// Flow actor file can include it without the C headers. Everything runs on one thread (Net2's), so there
// is no lock and no work queue. IPv4 only; addresses are host byte order.
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct QuicAddr {
	uint32_t ip = 0;
	uint16_t port = 0;
};

struct QuicOut {
	std::vector<uint8_t> bytes;
	QuicAddr to;
};

struct QuicStats {
	uint64_t sent = 0, sentBytes = 0, sendFailed = 0;
	uint64_t received = 0, receivedBytes = 0;
	uint64_t localError = 0, remoteError = 0, refused = 0; // refused: WebTransport CONNECT refused by the server
	uint64_t pktsIn = 0, pktsOut = 0, bytesOut = 0, inRcErr = 0;
	int lastInRc = 0;
	bool ready = false, closed = false;
	int state = -1; // picoquic_state_enum of the first connection at the end of the run, -1 when none
};

struct QuicRunCfg {
	std::string cert, key, root, sni;
	bool control = false; // client trusts the wrong root
	bool badpath = false; // WebTransport CONNECT to a path the server does not serve
	bool wt = false; // HTTP/3 + WebTransport (rung B) instead of raw QUIC datagrams (rung A)
	int count = 192; // datagrams
	int size = 1200; // bytes per datagram
};

struct QuicResult {
	QuicStats client, server;
	bool setupFailed = false;
	std::string setupError;
};

class QuicEndpoint {
public:
	QuicEndpoint();
	~QuicEndpoint();
	QuicEndpoint(const QuicEndpoint&) = delete;
	QuicEndpoint& operator=(const QuicEndpoint&) = delete;

	// Server: certificate and key, echoes every datagram it receives.
	bool initServer(const char* certFile, const char* keyFile, const char* alpn);
	// Client: verifies the server against rootFile, strictly (no root list means refuse, not skip).
	bool initClient(const char* rootFile, const char* alpn);
	bool connect(QuicAddr server, const char* sni);
	// Rung B: HTTP/3 with a WebTransport session at /echo; datagrams ride the session.
	bool initServerWt(const char* certFile, const char* keyFile);
	bool initClientWt(const char* rootFile);
	bool connectWt(QuicAddr server, const char* sni, const char* path);

	void feed(const uint8_t* bytes, size_t len, QuicAddr from, QuicAddr to);
	// Next packet picoquic wants sent, or false when it has none.
	bool pull(QuicOut& out);
	int64_t wakeDelayUs();
	bool queueDatagram(const uint8_t* bytes, size_t len);
	void snapshotState(); // fills stats.state
	void setWake(void (*fn)(void*), void* ctx) {
		wakeFn = fn;
		wakeCtx = ctx;
	}

	QuicStats stats;
	bool echo = false;
	std::string alpn;
	std::string connectError;
	bool wt = false;
	bool everSeen = false;
	std::string wtAuthority, wtPath;
	void (*wakeFn)(void*) = nullptr;
	void* wakeCtx = nullptr;
	struct Impl;
	Impl* impl;
};

// Prints the run and returns 0 when it did what its mode requires, 1 otherwise.
void quicDebugToStderr();
int quicJudge(const QuicRunCfg& cfg, const QuicResult& res);
