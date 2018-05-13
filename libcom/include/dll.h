#ifndef _SNF_DLL_H_
#define _SNF_DLL_H_

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <string>

namespace snf {

/**
 * Loads/unloads dynamically linked library (shared objects on Unix
 * platforms).
 */
class dll
{
private:
	std::string m_path;

#if defined(_WIN32)
	HMODULE     m_handle;
#else
	void        *m_handle;
#endif

public:
	dll() = delete;
	dll(const dll &) = delete;
	dll(dll &&) = delete;

	dll(const std::string &, bool lazy = false);
	~dll();
	
	void *symbol(const char *);
};

} // snf

#endif // _SNF_DLL_H_
