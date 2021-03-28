#include "dir.h"
#include "i18n.h"

namespace snf {

/*
 * Construct the directory object using the directory path and the pattern.
 * The files matching the pattern will be fetched later using read().
 * 
 * @param [in] path    the directory path
 * @param [in] pattern the file matching pattern.
 * 
 * @throws std::system_error, std::runtime_error
 */
dir_impl::dir_impl(const std::string &path, const std::string &pattern)
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

	std::unique_ptr<wchar_t []> pathW(mbs2wcs(m_path.c_str()));
	if (!pathW)
		throw std::runtime_error("invalid directory path");

	WIN32_FIND_DATAW fd;
	memset(&fd, 0, sizeof(fd));
	m_hdl = FindFirstFileW(pathW.get(), &fd);
	if (INVALID_HANDLE_VALUE == m_hdl) {
		std::ostringstream oss;
		oss << "failed to open directory " << m_path;
		throw std::system_error(snf::system_error(), std::system_category(), oss.str());
	} else {
		m_fa = file_attr(fd);
		if (!std::regex_match(m_fa.f_name, m_pattern))
			next();
	}
#else // !_WIN32
	m_dir = opendir(m_path.c_str());
	if (m_dir == nullptr) {
		std::ostringstream oss;
		oss << "failed to open directory " << m_path;
		throw std::system_error(snf::system_error(), std::system_category(), oss.str());
	} else {
		next();
	}
#endif
}

dir_impl::~dir_impl()
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

/*
 * Reads the next file entry matching the pattern specified
 * in the constructor. The file attributes of the matching
 * entry are stored in m_fa.
 * 
 * @throws std::system_error
 * 
 * @return true if a file is successfully read, false in case
 * of reaching end-of-directory.
 */
bool
dir_impl::next()
{
#if defined(_WIN32)
	DWORD syserr = ERROR_SUCCESS;
	WIN32_FIND_DATAW fd;

	while (true) {
		snf::system_error(0);
		memset(&fd, 0, sizeof(fd));

		if (!FindNextFileW(m_hdl, &fd)) {
			syserr = snf::system_error();
			break;
		}

		const wchar_t *name = fd.cFileName;

		if ((name[0] == L'.') && (name[1] == L'\0'))
			continue;

		if ((name[0] == L'.') && (name[1] == L'.') && (name[2] == L'\0'))
			continue;

		m_fa = file_attr(fd);

		if (std::regex_match(m_fa.f_name, m_pattern))
			break;
	}

	if (syserr == ERROR_NO_MORE_FILES) {
		m_fa.clear();
		syserr = ERROR_SUCCESS;
		return false;
	}
#else // !_WIN32
	int syserr = 0;
	struct dirent *pent;

	while (true) {
		snf::system_error(0);

		pent = readdir(m_dir);
		if (pent == nullptr) {
			syserr = snf::system_error();
			if (0 == syserr) {
				m_fa.clear();
				return false;
			}
		}

		const char *name = pent->d_name;

		if ((name[0] == '.') && (name[1] == '\0'))
			continue;

		if ((name[0] == '.') && (name[1] == '.') && (name[2] == '\0'))
			continue;

		if (!std::regex_match(name, m_pattern))
			continue;

		m_fa = file_attr(m_path, name);
		m_fa.f_name = name;
		break;
	}
#endif

	if (syserr != 0) {
		std::ostringstream oss;
		oss << "failed to read directory " << m_path;
		throw std::system_error(syserr, std::system_category(), oss.str());
	}

	return true;
}

} // namespace snf
