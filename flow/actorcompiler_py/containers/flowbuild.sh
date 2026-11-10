set -u
cd /work/fdbbuild
export CC=clang CXX=clang++ PYTHONDONTWRITEBYTECODE=1
cmake -S /src -B . -G Ninja -DCMAKE_BUILD_TYPE=Release -DFDB_USE_CSHARP_TOOLS=OFF -DUSE_LD=LLD -DWITH_TLS=OFF -DSSD_ROCKSDB_EXPERIMENTAL=OFF -DBUILD_DOCUMENTATION=OFF -DBUILD_TESTING=OFF -DWITH_SWIFT=OFF -DWITH_JAVA_BINDING=OFF -DWITH_GO_BINDING=OFF -DWITH_RUBY_BINDING=OFF -DWITH_PYTHON_BINDING=OFF -DUSE_JEMALLOC=OFF > configure.log 2>&1; echo "configure rc=$?"; tail -5 configure.log
cmake --build . --target flow --parallel 6 > build.log 2>&1; echo "build rc=$?"; grep -E "error|FAILED" build.log | head -15; tail -3 build.log
