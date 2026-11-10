Stacked on #12 (base `dev/flow-quic`). Adds a `windows-flowquic` entry to the portability matrix: the same clang-cl, Boost 1.86 and vcpkg setup as the windows entry, configured with `-DFLOWQUIC=ON`, building only `flow_quic_echo`. It publishes nothing and does not touch the desk.

Not yet shown to work: the picotls fetch has never run, and whether vcpkg's static OpenSSL is the one picoquic's `find_package` picks up is unknown. A check step fails if `FLOWQUIC` is not ON in the cache and prints the `OPENSSL_*` entries. `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` is there in case picotls declares an old minimum. The job builds the echo binary and does not run it.

A pull request into `dev/flow-quic` triggers no workflow, so the evidence is a `workflow_dispatch` run on this branch; its id is in the first comment.
