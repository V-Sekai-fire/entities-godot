"""Archive through a response file; godot-cpp's object list overflows the shell argument limit."""

import sys

import SCons.Tool.ar as _ar
from SCons.Platform import TempFileMunge


def generate(env):
    _ar.generate(env)
    if not sys.platform.startswith("linux"):
        return
    env["TEMPFILE"] = TempFileMunge
    env["TEMPFILEPREFIX"] = "@"
    env["MAXLINELENGTH"] = 32768
    env["ARCOM"] = '${TEMPFILE("$AR $ARFLAGS $TARGET $SOURCES","$ARCOMSTR")}'
    print("godot-cpp-build: ar uses a response file past 32768 bytes of command line")


def exists(env):
    return _ar.exists(env)
