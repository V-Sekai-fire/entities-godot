#!/bin/bash
cd /work/fdbbuild || exit 1
cmake --build . --target flow_quic_echo --parallel 6 2>&1 | grep -E "error|FAILED" | head
PQ=/pq/certs
for i in 1 2 3; do echo "=== positive $i"; timeout 60 bin/flow_quic_echo $PQ/cert.pem $PQ/key.pem $PQ/test-ca.crt test.example.com positive 2>&1 | grep -E 'PASS|FAIL|caught|^sockets|^packets' | sed 's/.*caught/caught/' | cut -c1-230; done
for i in 1 2; do echo "=== control $i"; timeout 60 bin/flow_quic_echo $PQ/cert.pem $PQ/key.pem $PQ/badcert.pem test.example.com control 2>&1 | grep -E 'PASS|FAIL|TLS alert|caught' | sed 's/.*caught/caught/' | cut -c1-200; done
