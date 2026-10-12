def can_build(env, platform):
    # Desktop only, where Movie Maker runs. The SDK does not build elsewhere: EncoderSDK/StdAfx.h
    # includes <uuid/uuid.h>, which the Android NDK lacks, and Codec/encoder.c includes
    # <Carbon/Carbon.h> on every Apple target, which iOS and visionOS lack.
    # The codec includes <emmintrin.h> unconditionally; arm64 reaches it through sse2neon.
    return platform in ["linuxbsd", "windows", "macos"] and env["arch"] in ["x86_64", "x86_32", "arm64"]


def configure(env):
    pass


def get_doc_classes():
    return ["VideoStreamCineForm", "VideoStreamPlaybackCineForm"]


def get_doc_path():
    return "doc_classes"
