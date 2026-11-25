// Opens libfdb_c for the stubs generated from fdb_c.sigs. weft_fdb_start() calls this before its
// first FoundationDB call; calling a stub that was never loaded would crash.
#include "modules/sqlite/fdb/fdb_c_stubs.h"
#include "modules/sqlite/fdb/fdb_c.h"

#include "core/os/os.h"

#include <string>
#include <vector>

namespace stubs = modules_sqlite_fdb;

extern "C" int weft_fdb_client_load(void) {
	static int result = -2; // -2: not tried yet
	if (result != -2) {
		return result;
	}
#if defined(_WIN32)
	const char *names[] = { "fdb_c.dll", "libfdb_c.dll" };
#elif defined(__APPLE__)
	const char *names[] = { "libfdb_c.dylib" };
#else
	const char *names[] = { "libfdb_c.so" };
#endif
	// Beside the binary first, so the client unpacks with the build; then the system search path
	// and, on macOS, the FoundationDB installer's directory.
	std::vector<std::string> paths;
	const String exe_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	for (const char *name : names) {
		if (!exe_dir.is_empty()) {
			paths.push_back((exe_dir + "/" + name).utf8().get_data());
		}
		paths.push_back(name);
#if defined(__APPLE__)
		paths.push_back(std::string("/usr/local/lib/") + name);
#endif
	}
	stubs::StubPathMap path_map;
	path_map[stubs::kModuleFdb_c] = paths;
	result = stubs::InitializeStubs(path_map) ? 0 : -1;
	return result;
}
