# The store: SQLite with a VFS whose pages live in FoundationDB.
#
# Two stages. The build stage carries the FoundationDB and SQLite headers;
# the runtime image carries neither compiler nor the sqlite3 shared library
# (the vendored SQLite in `thirdparty/sqlite/` is statically linked into
# every binary this tree produces).
#
# Rung 11 removed the iceoryx2 daemon path: workers link `libweft_fdb_vfs.a`
# directly and open `sqlite3*` in-process, so this Containerfile no longer
# needs a Rust toolchain stage or a runtime `/opt/iceoryx/lib` mount.
#
#   fly deploy --config fly/fly.toml
ARG FDB_VERSION=7.3.76

FROM docker.io/library/debian:bookworm-slim AS build
ARG FDB_VERSION
RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake make gcc g++ python3 libsqlite3-dev curl ca-certificates \
  && curl -fsSL -o /tmp/fdb.deb \
      "https://github.com/apple/foundationdb/releases/download/${FDB_VERSION}/foundationdb-clients_${FDB_VERSION}-1_amd64.deb" \
  && dpkg -i /tmp/fdb.deb && rm /tmp/fdb.deb \
  && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . /src
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j

FROM docker.io/library/debian:bookworm-slim
ARG FDB_VERSION
RUN apt-get update && apt-get install -y --no-install-recommends \
      libstdc++6 curl ca-certificates \
  && curl -fsSL -o /tmp/fdb.deb \
      "https://github.com/apple/foundationdb/releases/download/${FDB_VERSION}/foundationdb-clients_${FDB_VERSION}-1_amd64.deb" \
  && dpkg -i /tmp/fdb.deb && rm /tmp/fdb.deb \
  && apt-get purge -y curl && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/ /usr/local/lib/store/
ENV WEFT_FDB_CLUSTER_FILE=/etc/foundationdb/fdb.cluster
CMD ["/bin/sh", "-c", "ls /usr/local/lib/store"]
