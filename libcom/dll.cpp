#include "dll.h"
#include "util.h"

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

int
Dll::load(bool lazy)
{
	const char  *caller = "Dll::load";
	int         retval = E_ok;

#if defined(_WIN32)

	hModule = LoadLibrary(path.c_str());
	if (hModule == NULL) {
		Log(ERR, caller, GET_ERRNO,
			"failed to load library %s",
			path.c_str());
		retval = E_load_failed;
	}

#else

	int flags = RTLD_GLOBAL;

	if (lazy)
		flags |= RTLD_LAZY;
	else
		flags |= RTLD_NOW;

	handle = dlopen(path.c_str(), flags);
	if (handle == 0) {
		Log(ERR, caller,
			"failed to load library %s: %s",
			path.c_str(), dlerror());
		retval = E_load_failed;
	}

#endif

	return retval;
}

void *
Dll::getSymbol(const char *symbol)
{
	const char  *caller = "Dll::getSymbol";
	void        *addr = 0;

#if defined(_WIN32)

	if (hModule != NULL) {
		addr = GetProcAddress(hModule, symbol);
		if (addr == 0) {
			Log(ERR, caller, GET_ERRNO,
				"failed to find symbol %s in library %s",
				symbol, path.c_str());	
		}
	}

#else

	if (handle != 0) {
		addr = dlsym(handle, symbol);
		if (addr == 0) {
			Log(ERR, caller,
				"failed to find symbol %s in library %s: %s",
				symbol, path.c_str(), dlerror());
		}
	}

#endif

	return addr;
}

int
Dll::unload()
{
	const char  *caller = "Dll::unload";
	int         retval = E_ok;

#if defined(_WIN32)

	if (hModule != NULL) {
		if (!FreeLibrary(hModule)) {
			Log(ERR, caller, GET_ERRNO,
				"failed to unload library %s",
				path.c_str());
			retval = E_unload_failed;
		} else {
			hModule = NULL;
		}
	}

#else

	if (handle != 0) {
		if (dlclose(handle) != 0) {
			Log(ERR, caller,
				"failed to unload library %s: %s",
				path.c_str(), dlerror());
			retval = E_unload_failed;
		} else {
			handle = 0;
		}
	}

#endif

	return retval;
}
