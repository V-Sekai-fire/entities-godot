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
// POSIX declares char *dlerror(void), and the generated stubs bind its result to a char *.
static inline char *dlerror(void) {
	static char message[] = "LoadLibrary/GetProcAddress failed";
	return message;
}
#endif
