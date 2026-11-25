def can_build(env, platform):
    # The FoundationDB client is opened at run time (fdb/fdb_c.sigs), so no platform or architecture
    # is left out for want of a client. MSVC and clang-cl cannot compile the weft_fdb VFS, which
    # needs POSIX threads. Where the client library cannot be loaded, SQLite still works without it.
    return not getattr(env, "msvc", False)


def get_opts(platform):
    return []


def configure(env):
    pass


def get_doc_classes():
    return [
        "SQLite",
        "SQLiteQuery",
    ]


def get_doc_path():
    return "doc_classes"
