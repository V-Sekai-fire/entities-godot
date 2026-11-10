# The C# tools (actor compiler, vexillographer, coverage tool) need mono or a Visual Studio generator.
# With FDB_USE_CSHARP_TOOLS=OFF the actor compiler and vexillographer run as Python from this tree and the
# coverage tool is skipped, so Windows can build with Ninja and clang-cl.
if(WIN32)
  set(fdb_csharp_default OFF)
else()
  set(fdb_csharp_default ON)
endif()
option(FDB_USE_CSHARP_TOOLS "Build and use the C# tools instead of the Python ones" ${fdb_csharp_default})

if(NOT FDB_USE_CSHARP_TOOLS)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
elseif(WIN32)
  # C# is currently only supported on Windows with a Visual Studio generator.
  enable_language(CSharp)
else()
  # for other platforms we currently use mono

  find_program(MONO_EXECUTABLE mono)
  if (NOT MONO_EXECUTABLE)
    message(FATAL_ERROR "Could not find 'mono' executable!")
  endif()

  find_program(MCS_EXECUTABLE mcs)
  if (NOT MCS_EXECUTABLE)
    message(FATAL_ERROR "Could not find 'mcs' executable, which is part of Mono!")
  endif()
endif()
