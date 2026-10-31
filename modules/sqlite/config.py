def can_build(env, platform):
    return True


def get_opts(platform):
    from SCons.Variables import PathVariable

    return [
        PathVariable(
            "fdb_include",
            "Directory that holds the FoundationDB client header fdb_c.h",
            "/usr/include/foundationdb",
            PathVariable.PathAccept,
        ),
        PathVariable(
            "fdb_libpath",
            "Directory that holds the FoundationDB client library (libfdb_c)",
            "/usr/lib",
            PathVariable.PathAccept,
        ),
    ]


def configure(env):
    pass


def get_doc_classes():
    return [
        "SQLite",
        "SQLiteQuery",
    ]


def get_doc_path():
    return "doc_classes"
