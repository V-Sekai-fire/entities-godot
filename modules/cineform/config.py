def can_build(env, platform):
    # The CineForm codec includes <emmintrin.h> unconditionally; arm64 reaches it through sse2neon.
    return env["arch"] in ["x86_64", "x86_32", "arm64"]


def configure(env):
    pass


def get_doc_classes():
    return ["VideoStreamCineForm", "VideoStreamPlaybackCineForm"]


def get_doc_path():
    return "doc_classes"
