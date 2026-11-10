# flowquic, rung A: picoquic driven by Flow's Net2

`flowquic/` (option `FLOWQUIC`, default OFF) runs a picoquic server and client in one process on one Net2 loop. Flow owns both UDP sockets (`IUDPSocket::receiveFrom` / `sendTo`) and the clock; picoquic is a state machine fed by `picoquic_incoming_packet` and drained by `picoquic_prepare_next_packet_ex`. No picoquic packet loop, no thread, no lock. TLS is OpenSSL (flow already links it); RFD66606-2261 wants mbedTLS, so this is a recorded deviation. picoquic is pinned at a6f85e8a, unchanged.

Run: aarch64 Linux container, clang, loopback, `FDB_USE_CSHARP_TOOLS=OFF`. Recipes: `flowquic/build-in-container.sh`, `flowquic/run-in-container.sh`.

| Run | Runs | Result |
| --- | --- | --- |
| positive: client trusts `test-ca.crt`, 192 datagrams of 1200 bytes at 64 Hz | 3 | 3 of 3 passed: 192 of 192 datagrams echoed, handshake ok |
| control: client trusts `badcert.pem` | 2 | 2 of 2 passed: handshake refused, TLS alert 48 (unknown_ca), 0 datagrams received |

The control is not vacuous: the client uses strict verification, and the same binary passes the positive run.

## Bug found in the fork's Net2

Before the fix, the same positive run passed but the log showed 2 of 4 handshake packets per side dropped. `ReadPromise`'s move constructor did not move `endpoint`, so `UDPSocket::receiveFrom` wrote the sender into freed memory; the sender came back as a garbage IPv6 address and `toV4()` threw `bad_variant_access`, which ended the actor as `unknown_error`. QUIC retransmission hid it. After moving `endpoint` in the move constructor: 0 exceptions, every socket-level packet reaches picoquic (client 394 out, server 394 in).

## Not shown

x86_64 Linux, mbedTLS, a path MTU other than loopback, and the entity tick. The scenario's diagnostic counters are still in the source.

## Rung B: HTTP/3 and WebTransport on the same loop

Same driver; `BUILD_HTTP` on, `picohttp-core` linked, ALPN `h3`. The server serves a WebTransport CONNECT at `/echo` and echoes every session datagram; the client opens the session and counts the echoes. Run with the trailing `wt` argument.

| Run | Runs | Result |
| --- | --- | --- |
| positive, 192 datagrams of 1200 bytes at 64 Hz | 3 | 3 of 3 PASS: 192 of 192 echoed through the session |
| control, client trusts `badcert.pem` | 3 | 3 of 3 PASS: handshake refused, TLS alert 48, 0 datagrams |
| badpath, CONNECT to `/nope` | 3 | 3 of 3 PASS: refused once, session not opened, 0 datagrams |
| rung A positive (no `wt`) | 3 | 3 of 3 PASS: 192 of 192 |

The badpath control exercises the HTTP/3 layer, because TLS and QUIC succeed there. The socket counters stay in the output (client 396 sent, server 396 received in the positive run), and a sender that is not IPv4 is counted and reported, not thrown on. `FLOWQUIC_DEBUG=1` turns on picoquic's debug output.

Limits: loopback, OpenSSL. On aarch64 Linux the only build path tried is `FLOWQUIC_PICOQUIC_DIR` pointing at a local checkout of a6f85e8a; the Windows job below is the run of the `FetchContent` fallback. A WebTransport datagram adds the session id and a datagram frame header to the 1200-byte payload, so the packet on the wire is a little over 1200 bytes (not measured). Loopback reported a 1440-byte MTU; a real path decides what the entity view can be.

## Windows: clang-cl, Boost 1.86, vcpkg static OpenSSL

The `windows-flowquic` entry of `.github/workflows/fdb-portability.yml` configures with `-DFLOWQUIC=ON`, fetches picoquic a6f85e8a and (through `PICOQUIC_FETCH_PTLS`) picotls e446233b, builds the five libraries and `flow_quic_echo`, and runs it. OpenSSL is vcpkg's `x64-windows-static` 3.6.5. A dispatch with `only=windows-flowquic` runs just this entry (about 3 minutes with vcpkg and Boost cached).

picotls has no Windows handling in its CMake, so `flowquic/CMakeLists.txt` and `flowquic/ptls_clangcl_compat.h` supply it:
- the include path to `picotlsvs/picotls` (for `wincompat.h`), wrapped in `$<BUILD_INTERFACE:>` because picotls exports its targets;
- `wintimeofday.c` added to `picotls-core`, since `wincompat.h` maps `gettimeofday` to it;
- a force-included header that defines `PTLS_BUILD_ASSERT_EXPR` as picotls.h's own GNU definition (`picotls.h` defines it only under `__GNUC__`, which clang-cl does not set, while its array check `PTLS_ELEMENTSOF` is gated on `__clang__`; configure checks that an array is accepted and a pointer is refused) and keeps `__attribute__` (`wincompat.h` empties it, which breaks clang's intrinsic headers);
- `-Wno-error=incompatible-pointer-types` on `picotls-minicrypto` only, for cifra's `uint32_t*` passed to `_BitScanReverse`.
`pkgconfiglite` is installed on the runner because picotls requires `FIND_PACKAGE(PkgConfig REQUIRED)`.

Run 7 of the dispatch, three runs per case, every run exit 0 with a PASS line:

| Case | Runs | Result |
| --- | --- | --- |
| QUIC positive, 192 datagrams of 1200 bytes at 64 Hz | 3 | 3 of 3 PASS: 192 of 192 echoed |
| QUIC control, client trusts `badcert.pem` | 3 | 3 of 3 PASS: handshake refused, TLS alert 48, 0 datagrams |
| WebTransport positive | 3 | 3 of 3 PASS: 192 of 192 echoed |
| WebTransport control | 3 | 3 of 3 PASS: TLS alert 48, 0 datagrams |
| WebTransport bad path (`/nope`) | 3 | 3 of 3 PASS: refused once, session not opened, 0 datagrams |

Socket counters in the positive runs: client sent 396 and received 340, server sent 340 and received 396 (397/341 in one QUIC run); 0 non-IPv4 senders. The Linux runs above report 394 to 396 on the same case.

## cifra's count_trailing_zeroes on Windows, probed and patched

`deps/cifra/src/bitops.h` computed `31 - _BitScanReverse(x)` under `_WINDOWS`. `_BitScanReverse`
finds the HIGHEST set bit, so that is a leading-zero count, not trailing; a planted-answer probe
(`flowquic/ptls_cifra_ctz_probe.c`) measured 7 of 7 rows wrong (`count_trailing_zeroes(1)` returned
31, not 0). Nothing in this build calls it — picotls never compiles `ocb.c`, cifra's only caller —
so this was dead but wrong code, not an observed failure.

`flowquic/CMakeLists.txt` now patches the fetched source at configure time: `_BitScanForward` gives
the index of the LOWEST set bit, which is exactly a trailing-zero count, so the call and the return
are both changed (`_BitScanForward(&r, x); return r;`). The patch asserts the broken text is present
before replacing it, so a future picotls tag that already fixed this (or changed the file) fails the
configure loudly instead of silently no-op'ing or double-patching. The probe's expected values were
already the correct trailing-zero counts, so a clean probe run (0 of 7 wrong) is the proof.
