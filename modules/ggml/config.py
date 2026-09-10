def can_build(env, platform):
    # ggml compiles on every platform Godot targets. The Metal / Vulkan /
    # WebGPU backend selection is decided inside ggml's own source graph;
    # this module just vendors the sources and forwards flags.
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "GgmlEngine",
        "GgmlTensor",
    ]


def get_doc_path():
    return "doc_classes"
