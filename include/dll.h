#ifndef _SNF_DLL_H_
#define _SNF_DLL_H_

#include "common.h"

/**
 * Loads/unloads dynamically linked library (shared objects on Unix
 * platforms).
 */
class Dll
{
private:
	std::string path;

#if defined(_WIN32)
	HMODULE     hModule;
#else
	void        *handle;
#endif

public:
	/**
	 * Creates Dll object with the dll path.
	 * @param [in] path - dll path.
	 */
	Dll(const std::string &path)
		: path(path)
	{
#if defined(_WIN32)
		hModule = NULL;
#else
		handle = 0;
#endif
	}

	int load(bool);
	void *getSymbol(const char *);
	int unload();
};

#endif // _SNF_DLL_H_
