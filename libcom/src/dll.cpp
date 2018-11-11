#include "dll.h"
#include "i18n.h"
#include <sstream>

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

namespace snf {

dll::dll(const std::string &path, bool lazy)
	: m_path(path)
	, m_handle(0)
{
#if defined(_WIN32)
	wchar_t *wpath = mbs2wcs(m_path.c_str());
	m_handle = LoadLibraryW(wpath);
	DWORD syserr = GetLastError();
	delete [] wpath;

	if (m_handle == NULL) {
		std::ostringstream oss;
		oss << "failed to load dll " << m_path;
		throw std::system_error(syserr, std::system_category(), oss.str());
	}
#else
	int flags = RTLD_GLOBAL;

	if (lazy)
		flags |= RTLD_LAZY;
	else
		flags |= RTLD_NOW;

	m_handle = dlopen(m_path.c_str(), flags);
	if (m_handle == 0) {
		std::ostringstream oss;
		oss << "failed to load shared library " << m_path << ": " << dlerror();
		throw std::runtime_error(oss.str());
	}
#endif
}

void *
dll::symbol(const char *symbol, bool fatal)
{
	void *addr = nullptr;

#if defined(_WIN32)
	addr = GetProcAddress(m_handle, symbol);
	if ((addr == 0) && fatal) {
		std::ostringstream oss;
		oss << "failed to find symbol " << symbol << " in dll " << m_path;
		throw std::system_error(GetLastError(), std::system_category(), oss.str());
	}
#else
	addr = dlsym(m_handle, symbol);
	if ((addr == 0) && fatal) {
		std::ostringstream oss;
		oss << "failed to find symbol " << symbol << " in shared library " << m_path
			<< ": " << dlerror();
		throw std::runtime_error(oss.str());
	}
#endif

	return addr;
}

dll::~dll()
{
#if defined(_WIN32)
	if (FreeLibrary(m_handle))
		m_handle = 0;
#else
	if (dlclose(m_handle) == 0)
		m_handle = 0;
#endif
}

} // snf
