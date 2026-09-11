def can_build(env, platform):
    return env["rendering_device"]


def configure(env):
    pass


def get_doc_classes():
    return [
        "OrderIndependentTransparencyCompositorEffect",
        "OrderIndependentTransparencyGeometryRelay",
    ]


def get_doc_path():
    return "doc_classes"
