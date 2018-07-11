#include "dir.h"
#include "i18n.h"

namespace snf {

/*
 * Construct the directory object using the directory path and the pattern.
 * The files matching the pattern will be fetched later using read().
 * @param [in] path    the directory path
 * @param [in] pattern the file matching pattern. The default pattern is
 *                     .* meaning match every file name.
 * #throws std::system_error, std::runtime_error
 */
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

/* Finalizes the directory object. The system directory handle is closed. */
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

/*
 * Reads the next file entry matching the pattern specified
 * in the constructor. For each matching file name, the vistor
 * is invoked with the specified context and the file attribute
 * of the file read i.e. (*visitor)(arg, fa), where fa is the
 * file_attr type.
 * @param [in] visitor the pointer to the file visitor function.
 * @param [in] arg     the caller specific context/argument.
 * @throws std::system_error
 * @return true if a file is successfully read, false in case
 * of reaching end-of-directory.
 */
bool
directory::read(file_visitor visitor, void *arg)
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

		(visitor)(arg, fa);
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

		(visitor)(arg, fa);
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

static void
fill_vector(void *arg, const file_attr &fa)
{
	std::vector<file_attr> *favec = reinterpret_cast<std::vector<file_attr> *>(arg);
	favec->push_back(fa);
}

/*
 * Read all the files matching the pattern in directory
 * into the specified vector.
 * @param [in]  path    the directory path.
 * @param [in]  pattern the file matching pattern. The default pattern is
 *                      .* meaning match every file name.
 * @param [out] favec   the vector of file (file_attr) read.
 * @throws std::system_error, std::runtime_error.
 * @return true if successful and at least 1 file is read,
 * false otherwise.
 */
bool
read_directory(const std::string &path, const std::string &pattern, std::vector<file_attr> &favec)
{
	directory dir(path, pattern);
	while (dir.read(fill_vector, &favec)) ;
	return !favec.empty();
}

static void
find_newest(void *arg, const file_attr &fa)
{
	file_attr *pfa = reinterpret_cast<file_attr *>(arg);
	if (fa.f_type == file_type::regular) {
		if (fa.f_mtime > pfa->f_mtime) {
			*pfa = fa;
		}
	}
}

/*
 * Read the newest file matching the pattern in directory. This is
 * pretty rudimentary. It simply picks a regular file with the greatest
 * modification time.
 * @param [in]  path    the directory path.
 * @param [in]  pattern the file matching pattern. The default pattern is
 *                      .* meaning match every file name.
 * @param [out] fa      the file attribute of the matching file.
 * @throws std::system_error, std::runtime_error.
 * @return true if successful and the file is read,
 * false otherwise.
 */
bool
read_newest(const std::string &path, const std::string &pattern, file_attr &fa)
{
	file_attr tmp_fa;
	directory dir(path, pattern);
	while (dir.read(find_newest, &tmp_fa)) ;

	if (!tmp_fa.f_name.empty()) {
		fa = tmp_fa;
		return true;
	}

	return false;
}

} // namespace snf
