#include "common.h"
#include "filesystem.h"
#include "error.h"
#include "log.h"
#include "i18n.h"
#include <ctype.h>

#if !defined(_WIN32)
#include <pwd.h>
#endif

namespace snf {
namespace fs {

/**
 * Get home directory for the current user.
 *
 * @param [out] buf    - the buffer to retrieve the home dir.
 * @param [in]  buflen - the buffer length.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
get_home(char *buf, size_t buflen)
{
	int retval = E_ok;

#if defined(_WIN32)

	const char *ptr = getenv("USERPROFILE");
	if (ptr == 0) {
		ptr = getenv("HOMEDRIVE");
		if (ptr == 0) {
			retval = E_not_found;
		} else {
			strncpy(buf, ptr, buflen);
			ptr = getenv("HOMEPATH");
			if (ptr == 0) {
				retval = E_not_found;
			} else {
				strncat(buf, ptr, buflen);
			}
		}
	} else {
		strncpy(buf, ptr, buflen);
	}

#else

	uid_t id = geteuid();
	struct passwd *pwd = getpwuid(id);
	if (pwd == 0) {
		retval = E_syscall_failed;
	} else {
		strncpy(buf, pwd->pw_dir, buflen);
	}

#endif

	buf[buflen] = '\0';
	return retval;
}

/**
 * Determines if the file path specified exists or not. It could be a file
 * or a directory.
 *
 * @param [in]  path  - File path.
 * @param [out] oserr - OS error code.
 *
 * @return true if the file path exists, false otherwise.
 */
bool
exists(const char *path, int *oserr)
{
	bool pathExists = true;

	if (oserr) *oserr = 0;

	if ((path == 0) || (*path == '\0')) {
		return false;
	}

#if defined(_WIN32)

	DWORD   error = ERROR_FILE_NOT_FOUND;
	DWORD   fileAttr = INVALID_FILE_ATTRIBUTES;
	wchar_t *pathW = snf::mbs2wcs(path);

	if (pathW) {
		fileAttr = GetFileAttributesW(pathW);
		if (fileAttr == INVALID_FILE_ATTRIBUTES) {
			error = GET_ERRNO;
			if (oserr) *oserr = error;
		}
		delete [] pathW;
	}

	if (fileAttr == INVALID_FILE_ATTRIBUTES) {
		if ((error == ERROR_FILE_NOT_FOUND) ||
			(error == ERROR_PATH_NOT_FOUND)) {
			pathExists = false;
		}
	}

#else

	if (access(path, F_OK) < 0) {
		if (oserr) *oserr = GET_ERRNO;
		if (ENOENT == GET_ERRNO) {
			pathExists = false;
		}
	}

#endif

	return pathExists;
}

/**
 * Gets the file size.
 *
 * @param [in]  path  - file path
 * @param [out] oserr - OS error code.
 *
 * @return file size on success, -ve error code on failure.
 */
int64_t
size(const char *path, int *oserr)
{
	int64_t     fsize = -1L;

	if (oserr) *oserr = 0;

	if ((path == 0) || (*path == '\0')) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	WIN32_FILE_ATTRIBUTE_DATA   fad;
	wchar_t                     *pathW = snf::mbs2wcs(path);

	if (pathW) {
		if (!GetFileAttributesExW(pathW, GetFileExInfoStandard, &fad)) {
			if (oserr) *oserr = GET_ERRNO;
			fsize = E_stat_failed;
		} else {
			LARGE_INTEGER dummy;
			dummy.LowPart = fad.nFileSizeLow;
			dummy.HighPart = fad.nFileSizeHigh;
			fsize = int64_t(dummy.QuadPart);
		}

		delete [] pathW;
	} else {
		fsize = E_xlate_failed;
	}

#else

	struct stat stbuf;

	if (stat(path, &stbuf) < 0) {
		if (oserr) *oserr = errno;
		fsize = E_stat_failed;
	} else {
		fsize = int64_t(stbuf.st_size);
	}

#endif

	return fsize;
}

/**
 * Determines if the file path is an absolute path.
 * On Unix systems, any path that starts with '/' is considered an absolute
 * file path. On Windows, a file path prefixed by the following pattern
 * is considered as an absolute file path:
 *
 * <pre>
 * <alpha>:\\[\\*] like C:\\ or D:\\
 * \\\\?\\<alpha>:\\[\\*] like \\\\?\\C:\\ or \\\\?\\D:\\
 * \\\\?\\UNC\\[\\*]
 * \\\\?\\Volume{GUID}[\\*]
 * \\\\?\\GLOBALROOT[\\*]
 * \\\\[\\*]
 * </pre>
 *
 * @param [in] p  - File path specified
 *
 * @return 0 if the file path is relative, +ve number if the file path is
 * absolute (length of the file prefix, including the path separators,
 * that qualifies the path as absolute), and -ve error code on error
 * (like invalid file path).
 */
int
is_abs_path(const char *p)
{
	int i = 0;

#if defined(_WIN32)

	if (isalpha(p[i]) && (p[i + 1] == ':') && (p[i + 2] == pathsep())) {
		/* C:\ */
		i = 3;
	} else if ((p[i] == pathsep()) && (p[i + 1] == pathsep()) && (p[i + 2] == '?') && (p[i + 3] == pathsep())) {
		/* \\?\ */
		i = 4;
		if (isalpha(p[i]) && (p[i + 1] == ':') && (p[i + 2] == pathsep())) {
			/* \\?\C:\ */
			i += 3;
		} else if ((p[i] == 'U') && (p[i + 1] == 'N') && (p[i + 2] == 'C')) {
			/* \\?\UNC */
			i += 3;
		} else if (strncmp(p + i, "Volume{", 7) == 0) {
			/* \\?\Volume{ */
			i += 7;
			while (p[i] && p[i] != '}') {
				i++;
			}
		} else if (strncmp(p + i, "GLOBALROOT", 10) == 0) {
			/* \\?\GLOBALROOT */
			i++;
		} else {
			return E_invalid_arg;
		}
	} else if ((p[0] == pathsep()) && (p[1] == pathsep())) {
		/* \\ */
		i = 2;
	}

#endif

	while (p[i] == pathsep())
		i++;

	return i;
}

/**
 * Makes the specified directory. All the intermediate paths that do not
 * exist will be created.
 *
 * @param [in] dir    - Directory name.
 * @param [in] mode   - Directory mode (modified by umask of the process).
 *                      Ignored on Windows.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
mkdir(const char *dir, mode_t mode, int *oserr)
{
	int         retval = E_ok;
	const char  *ptr1;
	const char  *ptr2;
	char        buf[MAXPATHLEN + 1] = { 0 };

	if (oserr) *oserr = 0;

	if ((dir == 0) || (*dir == '\0')) {
		return E_invalid_arg;
	}

	ptr1 = dir;
	int i = is_abs_path(dir);
	if (i < 0) {
		return i;
	}

	strncat(buf, dir, i);

	ptr1 += i;

	do {
		if ((ptr1 == 0) || (*ptr1 == '\0'))
			break;

		if ((ptr2 = strchr(ptr1, pathsep())) != 0) {
			ptr2++;
			strncat(buf, ptr1, ptr2 - ptr1);
			ptr1 = ptr2;
		} else {
			strcat(buf, ptr1);
			ptr1 = 0;
		}

		if (exists(buf))
			continue;

#if defined(_WIN32)

		BOOL    status = FALSE;
		wchar_t *pathW = snf::mbs2wcs(buf);

		if (pathW) {
			if (!CreateDirectoryW(pathW, 0)) {
				if (oserr) *oserr = GET_ERRNO;
				retval = E_mkdir_failed;
			}

			delete [] pathW;
		} else {
			retval = E_xlate_failed;
		}

#else

		if (::mkdir(buf, mode) < 0) {
			if (oserr) *oserr = GET_ERRNO;
			retval = E_mkdir_failed;
		}

#endif

	} while (retval == E_ok);

	return retval;
}

/**
 * Renames a file. On Unix systems, system call rename() is used. On
 * Windows, MoveFileEx() is used with MOVEFILE_REPLACE_EXISTING flag.
 *
 * @param [in]  newName - new file name.
 * @param [in]  oldName - old file name.
 * @param [out] oserr   - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
rename(const char *newName, const char *oldName, int *oserr)
{
	int 	retval = E_ok;

	if (oserr) *oserr = 0;

	if ((newName == 0) || (*newName == '\0')) {
		return E_invalid_arg;
	}

	if ((oldName == 0) || (*oldName == '\0')) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	DWORD   flags = MOVEFILE_REPLACE_EXISTING;
	DWORD   attr = 0;
	wchar_t *oldNameW = 0;
	wchar_t *newNameW = 0;

	oldNameW = snf::mbs2wcs(oldName);
	if (oldNameW) {
		attr = GetFileAttributesW(oldNameW);
		if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			flags = 0;
		}

		newNameW = snf::mbs2wcs(newName);
		if (newNameW) {
			if (flags) {
				attr = GetFileAttributesW(newNameW);
				if (attr & FILE_ATTRIBUTE_DIRECTORY) {
					flags = 0;
				}
			}

			if (!MoveFileExW(oldNameW, newNameW, flags)) {
				if (oserr) *oserr = GET_ERRNO;
				retval = E_rename_failed;
			}
			delete [] newNameW;
		} else {
			retval = E_xlate_failed;
		}

		delete [] oldNameW;
	} else {
		retval = E_xlate_failed;
	}

#else /* !_WIN32 */

	if (::rename(oldName, newName) < 0)
	{
		if (oserr) *oserr = errno;
		retval = E_rename_failed;
	}

#endif

	return retval;
}

/**
 * Removes a file.
 *
 * @param [in]  f     - file name.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
remove_file(const char *f, int *oserr)
{
	int	retval = E_ok;

	if (oserr) *oserr = 0;

	if ((f == 0) || (*f == '\0')) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	wchar_t *fW = snf::mbs2wcs(f);

	if (fW) {
		if (!DeleteFileW(fW)) {
			if (oserr) *oserr = GET_ERRNO;
			retval = E_remove_failed;
		}

		delete [] fW;
	} else {
		retval = E_xlate_failed;
	}

#else

	if (unlink(f) != 0) {
		if (*oserr) *oserr = errno;
		retval = E_remove_failed;
	}

#endif

	return retval;
}

/**
 * Removes a directory. It must be empty.
 *
 * @param [in]  d     - directory name.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
remove_dir(const char *d, int *oserr)
{
	int	retval = E_ok;

	if (oserr) *oserr = 0;

	if ((d == 0) || (*d == '\0')) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	wchar_t *dW = snf::mbs2wcs(d);

	if (dW) {
		if (!RemoveDirectoryW(dW)) {
			if (oserr) *oserr = GET_ERRNO;
			retval = E_remove_failed;
		}

		delete [] dW;
	} else {
		retval = E_xlate_failed;
	}

#else 

	if (rmdir(d) != 0) {
		if (oserr) *oserr = errno;
		retval = E_remove_failed;
	}

#endif

	return retval;
}

} // namespace fs
} // namespace snf
