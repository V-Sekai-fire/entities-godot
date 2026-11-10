// Unit tests for quicJudge: the decision table that says whether a run did what its mode requires.
// No sockets, no picoquic connection, no Flow. Each mode has rows that must pass and rows that must
// fail, so a judge that always returned 0, or always 1, fails here.
#include "QuicDriver.h"

#include <cstdio>

static int failures = 0;
static int checks = 0;

static void expect(const char* name, int got, int want) {
	checks++;
	if (got != want) {
		failures++;
		std::printf("MISMATCH %s: judge returned %d, wanted %d\n", name, got, want);
	}
}

static QuicRunCfg cfgFor(const char* mode) {
	QuicRunCfg c;
	c.control = std::string(mode) == "control";
	c.badpath = std::string(mode) == "badpath";
	return c;
}

static QuicResult goodPositive(const QuicRunCfg& c) {
	QuicResult r;
	r.client.ready = true;
	r.client.sent = (uint64_t)c.count;
	r.client.received = (uint64_t)c.count;
	return r;
}

static QuicResult goodControl() {
	QuicResult r;
	r.client.closed = true;
	r.client.localError = 0x130; // TLS alert 48, unknown_ca
	return r;
}

static QuicResult goodBadpath() {
	QuicResult r;
	r.client.refused = 1;
	return r;
}

static void positive() {
	QuicRunCfg c = cfgFor("positive");
	expect("positive: all echoed", quicJudge(c, goodPositive(c)), 0);
	QuicResult r = goodPositive(c);
	r.client.received -= 1;
	expect("positive: one datagram lost", quicJudge(c, r), 1);
	r = goodPositive(c);
	r.client.sent -= 1;
	expect("positive: one datagram not sent", quicJudge(c, r), 1);
	r = goodPositive(c);
	r.client.ready = false;
	expect("positive: handshake not completed", quicJudge(c, r), 1);
	r = goodPositive(c);
	r.client.closed = true;
	expect("positive: closed at the end", quicJudge(c, r), 1);
	r = goodPositive(c);
	r.client.received += 1;
	expect("positive: more echoed than sent", quicJudge(c, r), 1);
	c.count = 3;
	expect("positive: a different count", quicJudge(c, goodPositive(c)), 0);
}

static void control() {
	QuicRunCfg c = cfgFor("control");
	expect("control: refused by a TLS alert", quicJudge(c, goodControl()), 0);
	QuicResult r = goodControl();
	r.client.localError = 0x100;
	expect("control: lowest TLS error", quicJudge(c, r), 0);
	r.client.localError = 0x1ff;
	expect("control: highest TLS error", quicJudge(c, r), 0);
	r.client.localError = 0x0ff;
	expect("control: error just below the TLS range", quicJudge(c, r), 1);
	r.client.localError = 0x200;
	expect("control: error just above the TLS range", quicJudge(c, r), 1);
	r = goodControl();
	r.client.closed = false;
	expect("control: not closed, a timeout rather than a refusal", quicJudge(c, r), 1);
	r = goodControl();
	r.client.ready = true;
	expect("control: handshake completed, so the control did not discriminate", quicJudge(c, r), 1);
	r = goodControl();
	r.client.received = 1;
	expect("control: a datagram came through", quicJudge(c, r), 1);
}

static void badpath() {
	QuicRunCfg c = cfgFor("badpath");
	expect("badpath: refused, nothing flowed", quicJudge(c, goodBadpath()), 0);
	QuicResult r = goodBadpath();
	r.client.refused = 0;
	expect("badpath: no refusal seen, only an absence of traffic", quicJudge(c, r), 1);
	r = goodBadpath();
	r.client.ready = true;
	expect("badpath: the session opened", quicJudge(c, r), 1);
	r = goodBadpath();
	r.server.received = 1;
	expect("badpath: the server received a datagram", quicJudge(c, r), 1);
	r = goodBadpath();
	r.client.sent = 1;
	expect("badpath: the client sent a datagram", quicJudge(c, r), 1);
}

static void setup() {
	const char* modes[] = { "positive", "control", "badpath" };
	for (const char* m : modes) {
		QuicRunCfg c = cfgFor(m);
		QuicResult r;
		r.setupFailed = true;
		r.setupError = "x";
		expect((std::string("setup failed in ") + m).c_str(), quicJudge(c, r), 1);
	}
}

int main() {
	positive();
	control();
	badpath();
	setup();
	std::printf("%d checks, %d failed\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
