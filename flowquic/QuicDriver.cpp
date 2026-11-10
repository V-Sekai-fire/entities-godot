#include "QuicDriver.h"

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

extern "C" {
#include "picoquic.h"
#include "picoquic_utils.h"
#include "h3zero.h"
#include "h3zero_common.h"
#include "pico_webtransport.h"
#include "demoserver.h"
}

struct QuicEndpoint::Impl {
	picoquic_quic_t* quic = nullptr;
	picoquic_cnx_t* cnx = nullptr;
	// rung B
	picohttp_server_path_item_t item;
	picohttp_server_parameters_t param;
	std::deque<std::vector<uint8_t>> pending; // datagrams waiting for picoquic's provide-datagram callback
	uint64_t controlStream = UINT64_MAX;
};

static sockaddr_in toSock(QuicAddr a) {
	sockaddr_in s;
	std::memset(&s, 0, sizeof s);
	s.sin_family = AF_INET;
	s.sin_addr.s_addr = htonl(a.ip);
	s.sin_port = htons(a.port);
	return s;
}

static int quicCallback(picoquic_cnx_t* cnx,
                        uint64_t,
                        uint8_t* bytes,
                        size_t length,
                        picoquic_call_back_event_t ev,
                        void* ctx,
                        void*) {
	QuicEndpoint* ep = static_cast<QuicEndpoint*>(ctx);
	switch (ev) {
	case picoquic_callback_datagram:
		ep->stats.received++;
		ep->stats.receivedBytes += length;
		if (ep->echo)
			picoquic_queue_datagram_frame(cnx, length, bytes);
		break;
	case picoquic_callback_ready:
		ep->stats.ready = true;
		break;
	case picoquic_callback_close:
	case picoquic_callback_application_close:
	case picoquic_callback_stateless_reset:
		ep->stats.closed = true;
		ep->stats.localError = picoquic_get_local_error(cnx);
		ep->stats.remoteError = picoquic_get_remote_error(cnx);
		if (ep->impl->cnx == cnx)
			ep->impl->cnx = nullptr;
		break;
	default:
		break;
	}
	return 0;
}

QuicEndpoint::QuicEndpoint() : impl(new Impl) {
	std::memset(&impl->item, 0, sizeof impl->item);
	std::memset(&impl->param, 0, sizeof impl->param);
}

QuicEndpoint::~QuicEndpoint() {
	if (impl->quic)
		picoquic_free(impl->quic);
	delete impl;
}

static void allowDatagrams(picoquic_quic_t* quic) {
	picoquic_tp_t tp = *picoquic_get_default_tp(quic);
	tp.max_datagram_frame_size = PICOQUIC_MAX_PACKET_SIZE;
	picoquic_set_default_tp(quic, &tp);
}

bool QuicEndpoint::initServer(const char* certFile, const char* keyFile, const char* alpnName) {
	alpn = alpnName;
	echo = true;
	uint8_t seed[PICOQUIC_RESET_SECRET_SIZE] = { 0 };
	impl->quic = picoquic_create(8,
	                             certFile,
	                             keyFile,
	                             nullptr,
	                             alpn.c_str(),
	                             quicCallback,
	                             this,
	                             nullptr,
	                             nullptr,
	                             seed,
	                             picoquic_current_time(),
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             0);
	if (!impl->quic)
		return false;
	allowDatagrams(impl->quic);
	return true;
}

bool QuicEndpoint::initClient(const char* rootFile, const char* alpnName) {
	alpn = alpnName;
	uint8_t seed[PICOQUIC_RESET_SECRET_SIZE] = { 0 };
	impl->quic = picoquic_create(8,
	                             nullptr,
	                             nullptr,
	                             rootFile,
	                             alpn.c_str(),
	                             quicCallback,
	                             this,
	                             nullptr,
	                             nullptr,
	                             seed,
	                             picoquic_current_time(),
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             0);
	if (!impl->quic)
		return false;
	picoquic_set_client_cert_verification_policy(impl->quic, 1);
	allowDatagrams(impl->quic);
	return true;
}

bool QuicEndpoint::connect(QuicAddr server, const char* sni) {
	sockaddr_in sa = toSock(server);
	impl->cnx = picoquic_create_client_cnx(impl->quic,
	                                       reinterpret_cast<sockaddr*>(&sa),
	                                       picoquic_current_time(),
	                                       0,
	                                       sni,
	                                       alpn.c_str(),
	                                       quicCallback,
	                                       this);
	// picoquic_create_client_cnx starts the connection itself; a second picoquic_start_client_cnx returns -1.
	// With strict verification and no usable root list it returns no connection, which is the refusal case.
	if (!impl->cnx) {
		connectError = "picoquic_create_client_cnx returned no connection (it refuses when strict verification is on "
		               "and no root certificate loaded)";
		return false;
	}
	return true;
}

void QuicEndpoint::feed(const uint8_t* bytes, size_t len, QuicAddr from, QuicAddr to) {
	std::vector<uint8_t> copy(bytes, bytes + len); // picoquic decrypts in place
	sockaddr_in f = toSock(from), t = toSock(to);
	int rc = picoquic_incoming_packet(impl->quic,
	                                  copy.data(),
	                                  copy.size(),
	                                  reinterpret_cast<sockaddr*>(&f),
	                                  reinterpret_cast<sockaddr*>(&t),
	                                  0,
	                                  0,
	                                  picoquic_current_time());
	stats.pktsIn++;
	stats.lastInRc = rc;
	if (rc != 0)
		stats.inRcErr++;
}

bool QuicEndpoint::pull(QuicOut& out) {
	uint8_t buf[PICOQUIC_MAX_PACKET_SIZE];
	size_t len = 0;
	sockaddr_storage to, from;
	std::memset(&to, 0, sizeof to);
	std::memset(&from, 0, sizeof from);
	int ifIndex = 0;
	picoquic_connection_id_t logCid;
	picoquic_cnx_t* last = nullptr;
	// send_msg_size NULL: one packet per call, no coalescing, so one buffer is one datagram on the wire.
	int rc = picoquic_prepare_next_packet_ex(
	    impl->quic, picoquic_current_time(), buf, sizeof buf, &len, &to, &from, &ifIndex, &logCid, &last, nullptr);
	if (rc != 0 || len == 0 || to.ss_family != AF_INET)
		return false;
	const sockaddr_in* s = reinterpret_cast<const sockaddr_in*>(&to);
	stats.pktsOut++;
	stats.bytesOut += len;
	out.bytes.assign(buf, buf + len);
	out.to.ip = ntohl(s->sin_addr.s_addr);
	out.to.port = ntohs(s->sin_port);
	return true;
}

int64_t QuicEndpoint::wakeDelayUs() {
	return picoquic_get_next_wake_delay(impl->quic, picoquic_current_time(), 100000);
}

bool QuicEndpoint::queueDatagram(const uint8_t* bytes, size_t len) {
	if (wt) {
		picoquic_cnx_t* c = picoquic_get_first_cnx(impl->quic);
		bool ok = c && impl->controlStream != UINT64_MAX;
		if (ok) {
			impl->pending.emplace_back(bytes, bytes + len);
			ok = h3zero_set_datagram_ready(c, impl->controlStream) == 0;
		}
		if (ok) {
			stats.sent++;
			stats.sentBytes += len;
		} else {
			stats.sendFailed++;
		}
		if (wakeFn)
			wakeFn(wakeCtx);
		return ok;
	}
	bool ok = impl->cnx && picoquic_queue_datagram_frame(impl->cnx, len, bytes) == 0;
	if (ok) {
		stats.sent++;
		stats.sentBytes += len;
	} else {
		stats.sendFailed++;
	}
	if (wakeFn)
		wakeFn(wakeCtx);
	return ok;
}

void QuicEndpoint::snapshotState() {
	picoquic_cnx_t* c = (!wt && impl->cnx) ? impl->cnx : picoquic_get_first_cnx(impl->quic);
	if (c) {
		everSeen = true;
		picoquic_state_enum st = picoquic_get_cnx_state(c);
		stats.state = (int)st;
		// In WebTransport mode h3zero owns the connection callback, so close is read from the state instead.
		if (wt && (st >= picoquic_state_disconnecting || st == picoquic_state_handshake_failure ||
		           st == picoquic_state_handshake_failure_resend)) {
			stats.closed = true;
			stats.localError = picoquic_get_local_error(c);
			stats.remoteError = picoquic_get_remote_error(c);
		}
	} else if (wt && everSeen) {
		stats.closed = true;
	}
}

void quicDebugToStderr() {
	debug_printf_push_stream(stderr);
}

// ---- rung B: HTTP/3 + WebTransport (h3zero) -------------------------------------------------------------
// One path callback serves both ends. The server accepts a CONNECT at /echo and echoes every session datagram;
// the client counts what comes back. Datagrams leave through picoquic's provide-datagram callback, so they are
// queued here and the session is marked ready.
static int wtCallback(picoquic_cnx_t* cnx,
                      uint8_t* bytes,
                      size_t length,
                      picohttp_call_back_event_t ev,
                      struct st_h3zero_stream_ctx_t* sc,
                      void* ctx) {
	QuicEndpoint* ep = static_cast<QuicEndpoint*>(ctx);
	switch (ev) {
	case picohttp_callback_connect: { // server: a CONNECT arrived on a served path; accept it
		h3zero_callback_ctx_t* h3 = static_cast<h3zero_callback_ctx_t*>(picoquic_get_callback_context(cnx));
		if (!picowt_is_webtransport_connect(sc) || h3zero_declare_stream_prefix(h3, sc->stream_id, wtCallback, ep) != 0)
			return -1;
		sc->path_callback = wtCallback;
		sc->path_callback_ctx = ep;
		ep->impl->controlStream = sc->stream_id;
		return 0;
	}
	case picohttp_callback_connect_accepted: // client
		ep->stats.ready = true;
		return 0;
	case picohttp_callback_connect_refused: // client
		ep->stats.refused++;
		picowt_uncount_session(static_cast<h3zero_callback_ctx_t*>(picoquic_get_callback_context(cnx)), sc);
		return 0;
	case picohttp_callback_post_datagram:
		ep->stats.received++;
		ep->stats.receivedBytes += length;
		if (ep->echo) {
			ep->impl->pending.emplace_back(bytes, bytes + length);
			h3zero_set_datagram_ready(cnx, sc->stream_id);
		}
		return 0;
	case picohttp_callback_provide_datagram: { // bytes is picoquic's context, length is the space left
		if (ep->impl->pending.empty())
			return 0;
		const std::vector<uint8_t>& front = ep->impl->pending.front();
		uint8_t* buf = h3zero_provide_datagram_buffer(bytes, front.size(), ep->impl->pending.size() > 1);
		if (buf) {
			std::memcpy(buf, front.data(), front.size());
			ep->impl->pending.pop_front();
		} else {
			h3zero_provide_datagram_buffer(bytes, 0, 1); // too big for the path right now: stay ready, retry
		}
		return 0;
	}
	default:
		return 0;
	}
}

bool QuicEndpoint::initServerWt(const char* certFile, const char* keyFile) {
	wt = true;
	echo = true;
	alpn = "h3";
	impl->item.path = "/echo";
	impl->item.path_length = 5;
	impl->item.path_callback = wtCallback;
	impl->item.path_app_ctx = this;
	impl->item.connect_protocol = H3ZERO_WEBTRANSPORT_H3_PROTOCOL;
	impl->item.connect_protocol_length = sizeof(H3ZERO_WEBTRANSPORT_H3_PROTOCOL) - 1;
	impl->item.origin_validator = h3zero_origin_validator_allow_all;
	impl->param.path_table = &impl->item;
	impl->param.path_table_nb = 1;
	uint8_t seed[PICOQUIC_RESET_SECRET_SIZE] = { 0 };
	impl->quic = picoquic_create(8,
	                             certFile,
	                             keyFile,
	                             nullptr,
	                             "h3",
	                             h3zero_callback,
	                             &impl->param,
	                             nullptr,
	                             nullptr,
	                             seed,
	                             picoquic_current_time(),
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             0);
	if (!impl->quic)
		return false;
	picoquic_set_alpn_select_fn_v2(impl->quic, picoquic_demo_server_callback_select_alpn);
	allowDatagrams(impl->quic);
	picowt_set_default_transport_parameters(impl->quic);
	return true;
}

bool QuicEndpoint::initClientWt(const char* rootFile) {
	wt = true;
	alpn = "h3";
	uint8_t seed[PICOQUIC_RESET_SECRET_SIZE] = { 0 };
	impl->quic = picoquic_create(8,
	                             nullptr,
	                             nullptr,
	                             rootFile,
	                             "h3",
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             seed,
	                             picoquic_current_time(),
	                             nullptr,
	                             nullptr,
	                             nullptr,
	                             0);
	if (!impl->quic)
		return false;
	picoquic_set_client_cert_verification_policy(impl->quic, 1);
	allowDatagrams(impl->quic);
	picowt_set_default_transport_parameters(impl->quic);
	return true;
}

bool QuicEndpoint::connectWt(QuicAddr server, const char* sni, const char* path) {
	wtAuthority = sni;
	wtPath = path;
	sockaddr_in sa = toSock(server);
	picoquic_cnx_t* cnx = nullptr;
	h3zero_callback_ctx_t* h3 = nullptr;
	h3zero_stream_ctx_t* control = nullptr;
	if (picowt_prepare_client_cnx(
	        impl->quic, reinterpret_cast<sockaddr*>(&sa), &cnx, &h3, &control, picoquic_current_time(), sni) != 0) {
		connectError = "picowt_prepare_client_cnx failed";
		return false;
	}
	impl->cnx = cnx;
	impl->controlStream = control->stream_id;
	if (picowt_connect(cnx, h3, control, wtAuthority.c_str(), wtPath.c_str(), wtCallback, this, nullptr) != 0) {
		connectError = "picowt_connect failed";
		return false;
	}
	if (picoquic_start_client_cnx(cnx) != 0) {
		connectError = "picoquic_start_client_cnx returned non-zero (it refuses when strict verification is on and no "
		               "root certificate loaded)";
		return false;
	}
	return true;
}

int quicJudge(const QuicRunCfg& cfg, const QuicResult& res) {
	const QuicStats& c = res.client;
	const QuicStats& s = res.server;
	std::printf("mode=%s transport=%s datagram=%d bytes planned=%d datagrams\n",
	            cfg.badpath ? "badpath" : (cfg.control ? "control" : "positive"),
	            cfg.wt ? "h3+webtransport" : "quic",
	            cfg.size,
	            cfg.count);
	if (res.setupFailed) {
		std::printf("FAIL: setup: %s\n", res.setupError.c_str());
		return 1;
	}
	std::printf("client: handshake %s, sent %llu datagrams (%llu bytes), %llu not queued, received %llu datagrams "
	            "(%llu bytes), closed=%d local error 0x%llx remote error 0x%llx\n",
	            c.ready ? "ok" : "not completed",
	            (unsigned long long)c.sent,
	            (unsigned long long)c.sentBytes,
	            (unsigned long long)c.sendFailed,
	            (unsigned long long)c.received,
	            (unsigned long long)c.receivedBytes,
	            c.closed ? 1 : 0,
	            (unsigned long long)c.localError,
	            (unsigned long long)c.remoteError);
	std::printf("connection state at end: client %d, server %d (picoquic_state_ready is %d)\n",
	            c.state,
	            s.state,
	            (int)picoquic_state_ready);
	std::printf("packets: client out %llu (%llu bytes) in %llu (rc errors %llu, last rc %d); server out %llu in %llu "
	            "(rc errors %llu, last rc %d)\n",
	            (unsigned long long)c.pktsOut,
	            (unsigned long long)c.bytesOut,
	            (unsigned long long)c.pktsIn,
	            (unsigned long long)c.inRcErr,
	            c.lastInRc,
	            (unsigned long long)s.pktsOut,
	            (unsigned long long)s.pktsIn,
	            (unsigned long long)s.inRcErr,
	            s.lastInRc);
	std::printf("server: received %llu datagrams (%llu bytes)\n",
	            (unsigned long long)s.received,
	            (unsigned long long)s.receivedBytes);
	if (cfg.badpath) {
		// Control for the h3 layer: TLS and QUIC succeed, the server answers the CONNECT with an error status, and no
		// datagram may flow. Passing needs the refusal to have been seen, not merely an absence of traffic.
		bool ok = c.refused > 0 && !c.ready && c.received == 0 && c.sent == 0 && s.received == 0;
		std::printf("%s: badpath refused %llu time(s), session %s, received %llu datagrams\n",
		            ok ? "PASS" : "FAIL",
		            (unsigned long long)c.refused,
		            c.ready ? "accepted (control did not discriminate)" : "not opened",
		            (unsigned long long)c.received);
		return ok ? 0 : 1;
	}
	if (!cfg.control) {
		bool ok = c.ready && c.sent == (uint64_t)cfg.count && c.received == (uint64_t)cfg.count && !c.closed;
		std::printf("%s: received %llu of %d datagrams of %d bytes\n",
		            ok ? "PASS" : "FAIL",
		            (unsigned long long)c.received,
		            cfg.count,
		            cfg.size);
		return ok ? 0 : 1;
	}
	// Control: the client trusts the wrong root. It must not complete the handshake, must receive nothing, and
	// must have been closed by a TLS alert (QUIC crypto error range 0x100 to 0x1ff), not by a timeout or a typo.
	bool tls = c.localError >= 0x100 && c.localError <= 0x1ff;
	bool ok = !c.ready && c.received == 0 && c.closed && tls;
	if (tls)
		std::printf("control closed by TLS alert %llu\n", (unsigned long long)(c.localError - 0x100));
	std::printf("%s: control received %llu datagrams, handshake %s, tls alert %s\n",
	            ok ? "PASS" : "FAIL",
	            (unsigned long long)c.received,
	            c.ready ? "completed (control did not discriminate)" : "refused",
	            tls ? "yes" : "no");
	return ok ? 0 : 1;
}
