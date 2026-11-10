#!/bin/bash
set -u
cd /work/fdbbuild || exit 1
export CC=clang CXX=clang++ PYTHONDONTWRITEBYTECODE=1
cmake -S /src -B . -G Ninja -DFLOWQUIC=ON -DFLOWQUIC_PICOQUIC_DIR=/pq > quic_configure.log 2>&1; echo "configure rc=$?"; grep -E "CMake Error|picotls|Could NOT" -A4 quic_configure.log | head -20
cmake --build . --target flow_quic_echo --parallel 6 > quic_build.log 2>&1; echo "build rc=$?"; grep -E "error:|FAILED" quic_build.log | head -20; tail -3 quic_build.log
ls -la bin/flow_quic_echo 2>/dev/null
PQ=/pq/certs
if [ -x bin/flow_quic_echo ]; then
  echo "=== positive"; timeout 60 bin/flow_quic_echo $PQ/cert.pem $PQ/key.pem $PQ/test-ca.crt test.example.com positive; echo "positive rc=$?"
  echo "=== control (wrong root)"; timeout 60 bin/flow_quic_echo $PQ/cert.pem $PQ/key.pem $PQ/badcert.pem test.example.com control; echo "control rc=$?"
fi
