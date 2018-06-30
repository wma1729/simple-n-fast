#include "dir.h"
#include "i18n.h"

namespace snf {

directory::directory(const std::string &path, const std::string &pattern)
	: m_path(path)
	, m_pattern(pattern)
{
#if defined(_WIN32)

	if (m_path.empty()) {
		m_path += pathsep();
		m_path += '*';
	} else {
		if (m_path.at(m_path.size() - 1) == pathsep()) {
			m_path += '*';
		} else {
			m_path += pathsep();
			m_path += '*';
		}
	}

	wchar_t *pathW = mbs2wcs(m_path.c_str());
	if (pathW == nullptr)
		throw std::runtime_error("invalid directory path");

	memset(&m_fd, 0, sizeof(m_fd));
	m_hdl = FindFirstFileW(pathW, &m_fd);
	if (INVALID_HANDLE_VALUE == m_hdl) {
		delete [] pathW;
		std::ostringstream oss;
		oss << "failed to open directory " << m_path;
		throw std::system_error(snf::system_error(), std::system_category(), oss.str());
	}

	delete [] pathW;

#else // !_WIN32

	m_dir = opendir(m_path.c_str());
	if (m_dir == nullptr) {
		std::ostringstream oss;
		oss << "failed to open directory " << m_path;
		throw std::system_error(snf::system_error(), std::system_category(), oss.str());
	}

#endif
}

directory::~directory()
{
#if defined(_WIN32)

	if (INVALID_HANDLE_VALUE != m_hdl) {
		FindClose(m_hdl);
		m_hdl = INVALID_HANDLE_VALUE;
	}

#else // !_WIN32

	if (m_dir != nullptr) {
		closedir(m_dir);
		m_dir = nullptr;
	}

#endif
}

bool
directory::read(file_visitor visitor)
{
	snf::system_error(0);

#if defined(_WIN32)

	DWORD syserr;

	while (true) {
		if (m_first_read) {
			m_first_read = false;
		} else {
			memset(&m_fd, 0, sizeof(m_fd));
			if (!FindNextFileW(m_hdl, &m_fd)) {
				syserr = snf::system_error();
				break;
			}
		}

		const wchar_t *name = m_fd.cFileName;

		if ((name[0] == L'.') && (name[1] == L'\0'))
			continue;

		if ((name[0] == L'.') && (name[1] == L'.') && (name[2] == L'\0'))
			continue;

		file_attr fa(m_fd);

		if (!std::regex_match(fa.f_name, m_pattern))
			continue;

		(visitor)(fa);
		return true;
	}

	if (syserr == ERROR_NO_MORE_FILES)
		syserr = ERROR_SUCCESS;

#else // !_WIN32

	int syserr;
	struct dirent *pent;

	while ((pent = readdir(m_dir)) != nullptr) {
		if (pent == nullptr)
			break;

		const char *name = pent->d_name;

		if ((name[0] == '.') && (name[1] == '\0'))
			continue;

		if ((name[0] == '.') && (name[1] == '.') && (name[2] == '\0'))
			continue;

		if (!std::regex_match(name, m_pattern))
			continue;

		file_attr fa(m_path, name);
		fa.f_name = name;

		(visitor)(fa);
		return true;
	}

	syserr = snf::system_error();

#endif

	if (syserr != 0) {
		std::ostringstream oss;
		oss << "failed to read directory " << m_path;
		throw std::system_error(syserr, std::system_category(), oss.str());
	}

	return false;
}

} // namespace snf
