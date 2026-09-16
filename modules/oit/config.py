def can_build(env, platform):
    return env["vulkan"] and not env["disable_3d"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "OITCompositorEffect",
    ]


def get_doc_path():
    return "doc_classes"
