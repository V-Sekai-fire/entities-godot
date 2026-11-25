// dlopen for the generated stubs on MinGW, which has no <dlfcn.h>.
#pragma once

#ifdef _WIN32
#include <windows.h>
#define RTLD_LAZY 1
static inline void *dlopen(const char *path, int flags) {
	(void)flags;
	return (void *)LoadLibraryA(path);
}
static inline void *dlsym(void *handle, const char *name) {
	return (void *)GetProcAddress((HMODULE)handle, name);
}
static inline int dlclose(void *handle) {
	return FreeLibrary((HMODULE)handle) ? 0 : 1;
}
static inline const char *dlerror(void) {
	return "LoadLibrary/GetProcAddress failed";
}
#endif
