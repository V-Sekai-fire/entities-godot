def can_build(env, platform):
    # The FoundationDB client is published for x86_64 Linux, x86_64 Windows and arm64 macOS, and
    # the weft_fdb VFS needs POSIX threads, so MSVC and clang-cl cannot compile it.
    arch = env.get("arch", "")
    if platform == "linuxbsd":
        return arch == "x86_64"
    if platform == "windows":
        return arch == "x86_64" and not getattr(env, "msvc", False)
    if platform == "macos":
        return arch == "arm64"
    return False


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
