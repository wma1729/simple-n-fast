#include "common.h"
#include "filesystem.h"
#include "error.h"
#include "i18n.h"
#include "util.h"
#include <ctype.h>

#if !defined(WINDOWS)
#include <pwd.h>
#endif

/**
 * Get home directory for the current user.
 *
 * @param [out] buf    - the buffer to retrieve the home dir.
 * @param [in]  buflen - the buffer length.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FileSystem::getHome(char *buf, size_t buflen)
{
	int retval = E_ok;

#if defined(WINDOWS)

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
FileSystem::exists(const char *path, int *oserr)
{
	bool pathExists = true;

	if (oserr) *oserr = 0;

	if ((path == 0) || (*path == '\0')) {
		return false;
	}

#if defined(WINDOWS)

	DWORD   error = ERROR_FILE_NOT_FOUND;
	DWORD   fileAttr = INVALID_FILE_ATTRIBUTES;
	wchar_t *pathW = MbsToWcs(path);

	if (pathW) {
		fileAttr = GetFileAttributesW(pathW);
		if (fileAttr == INVALID_FILE_ATTRIBUTES) {
			error = GET_ERRNO;
			if (oserr) *oserr = error;
		}
		free(pathW);
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
FileSystem::isAbsolutePath(const char *p)
{
	int i = 0;

#if defined(WINDOWS)

	if (isalpha(p[i]) && (p[i + 1] == ':') && (p[i + 2] == PATH_SEP)) {
		/* C:\ */
		i = 3;
	} else if ((p[i] == PATH_SEP) && (p[i + 1] == PATH_SEP) && (p[i + 2] == '?') && (p[i + 3] == PATH_SEP)) {
		/* \\?\ */
		i = 4;
		if (isalpha(p[i]) && (p[i + 1] == ':') && (p[i + 2] == PATH_SEP)) {
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
	} else if ((p[0] == PATH_SEP) && (p[1] == PATH_SEP)) {
		/* \\ */
		i = 2;
	}

#endif

	while (p[i] == PATH_SEP)
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
FileSystem::mkdir(const char *dir, mode_t mode, int *oserr)
{
	const char  *caller = "FileSystem::mkdir";
	int         retval = E_ok;
	const char  *ptr1;
	const char  *ptr2;
	char        buf[MAXPATHLEN + 1] = { 0 };

	if (oserr) *oserr = 0;

	if ((dir == 0) || (*dir == '\0')) {
		return E_invalid_arg;
	}

	ptr1 = dir;
	int i = isAbsolutePath(dir);
	if (i < 0) {
		return i;
	}

	strncat(buf, dir, i);

	ptr1 += i;

	do {
		if ((ptr1 == 0) || (*ptr1 == '\0'))
			break;

		if ((ptr2 = strchr(ptr1, PATH_SEP)) != 0) {
			ptr2++;
			strncat(buf, ptr1, ptr2 - ptr1);
			ptr1 = ptr2;
		} else {
			strcat(buf, ptr1);
			ptr1 = 0;
		}

		if (FileSystem::exists(buf))
			continue;

		Log(DBG, caller, "making directory %s", buf);

#if defined(WINDOWS)

		BOOL    status = FALSE;
		wchar_t *pathW = MbsToWcs(buf);

		if (pathW) {
			status = CreateDirectoryW(pathW, 0);
			if (status == FALSE) {
				int error = GET_ERRNO;
				Log(ERR, caller, error, "CreateDirectory(%s) failed", buf);
				if (oserr) *oserr = error;
			}
			free(pathW);
		}

		if (status == FALSE) {
			retval = E_mkdir_failed;
		}

#else

		if (::mkdir(buf, mode) < 0) {
			int error = GET_ERRNO;
			Log(ERR, caller, error, "mkdir(%s) failed", buf);
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
FileSystem::rename(const char *newName, const char *oldName, int *oserr)
{
	const char	*caller = "FileSystem::rename";
	int 		retval = E_ok;

	if (oserr) *oserr = 0;

	if ((newName == 0) || (*newName == '\0')) {
		Log(ERR, caller, "invalid new name specified");
		return E_invalid_arg;
	}

	if ((oldName == 0) || (*oldName == '\0')) {
		Log(ERR, caller, "invalid old name specified");
		return E_invalid_arg;
	}

#if defined(WINDOWS)

	DWORD   flags = MOVEFILE_REPLACE_EXISTING;
	DWORD   attr = 0;
	wchar_t *oldNameW = 0;
	wchar_t *newNameW = 0;

	retval = E_rename_failed;

	oldNameW = MbsToWcs(oldName);
	if (oldNameW) {
		attr = GetFileAttributesW(oldNameW);
		if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			flags = 0;
		}

		newNameW = MbsToWcs(newName);
		if (newNameW) {
			if (flags) {
				attr = GetFileAttributesW(newNameW);
				if (attr & FILE_ATTRIBUTE_DIRECTORY) {
					flags = 0;
				}
			}

			if (MoveFileExW(oldNameW, newNameW, flags)) {
				retval = E_ok;
			} else {
				int status = GET_ERRNO;
				if (oserr) *oserr = status;
				Log(ERR, caller, status,
					"MoveFile(%s, %s) failed",
					oldName, newName);
			}
			free(newNameW);
		}
		free(oldNameW);
	}

#else /* !WINDOWS */

	if (::rename(oldName, newName) < 0)
	{
		if (oserr) *oserr = errno;
		Log(ERR, caller, errno, "rename(%s, %s) failed",
			oldName, newName);
		retval = E_rename_failed;
	}

#endif

	return retval;
}
