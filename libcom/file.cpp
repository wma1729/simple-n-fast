#include "file.h"
#include "i18n.h"
#include "error.h"

/**
 * Opens the file. Before opening the file, the umask
 * specified in the constructor is applied.
 *
 * @param [in] flags -  File open flags.
 * @param [in] mode  -  File open mode. This mode is
 *                      same as the mode in the
 *                      open(2) system call. Ignored
 *                      on Windows.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
File::open(const FileOpenFlags &flags, mode_t mode, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

#if defined(_WIN32)

	DWORD	amode = READ_CONTROL | SYNCHRONIZE;
	DWORD	shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD	disposition = OPEN_EXISTING;
	DWORD	fattr = FILE_ATTRIBUTE_NORMAL;

	if (flags.o_append) {
		amode |= (FILE_APPEND_DATA |
				  FILE_WRITE_ATTRIBUTES |
				  FILE_WRITE_EA |
				  WRITE_DAC |
				  WRITE_OWNER);
	} else if (flags.o_read && flags.o_write) {
		amode |= (FILE_READ_DATA |
				  FILE_READ_ATTRIBUTES |
				  FILE_READ_EA |
				  FILE_WRITE_DATA |
				  FILE_WRITE_ATTRIBUTES |
				  FILE_WRITE_EA |
				  WRITE_DAC |
				  WRITE_OWNER);
	} else if (flags.o_write) {
		amode |= (FILE_WRITE_DATA |
				  FILE_WRITE_ATTRIBUTES |
				  FILE_WRITE_EA |
				  WRITE_DAC |
				  WRITE_OWNER);
	} else {
		amode |= (FILE_READ_DATA |
				  FILE_READ_ATTRIBUTES |
				  FILE_READ_EA);
	}

	if (flags.o_create) {
		if (flags.o_excl) {
			disposition = CREATE_NEW;
		} else if (flags.o_truncate) {
			disposition = CREATE_ALWAYS;
		} else {
			disposition = OPEN_ALWAYS;
		}
	}

	if (flags.o_sync) {
		fattr = FILE_FLAG_WRITE_THROUGH;
	}

	wchar_t *fnameW = snf::mbs2wcs(fname.c_str());
	if (fnameW) {
		SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

		fd = CreateFileW(fnameW, amode, shareMode, &sa, disposition, fattr, NULL);

		delete [] fnameW;
	}

#else

	int oflags;

	if (flags.o_append) {
		oflags = O_APPEND | O_RDWR;
	} else if (flags.o_read && flags.o_write) {
		oflags = O_RDWR;
	} else if (flags.o_write) {
		oflags = O_WRONLY;
	} else {
		oflags = O_RDONLY;
	}

	if (flags.o_create) {
		oflags |= O_CREAT;
	}

	if (flags.o_excl) {
		oflags |= O_EXCL;
	}

	if (flags.o_truncate) {
		oflags |= O_TRUNC;
	}

	if (flags.o_sync) {
		oflags |= O_SYNC;
	}

	FileMask fmask(mask);

	fd = ::open(fname.c_str(), oflags, mode);

#endif

	if (fd == INVALID_HANDLE_VALUE) {
		retval = E_open_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

	return retval;
}

/**
 * Reads from the file.
 *
 * @param [out] buf    - Buffer to read the data into.
 * @param [in]  toRead - Number of bytes to read.
 * @param [out] bRead  - Number of bytes read. This
 *                       can be less than toRead.
 * @param [out] oserr  - OS error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
File::read(void *buf, int toRead, int *bRead, int *oserr)
{
	int     retval = E_ok;
	int     n = 0, nbytes = 0;
	char    *cbuf = static_cast<char *>(buf);

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (buf == 0) {
		return E_invalid_arg;
	}

	if (toRead <= 0) {
		return E_invalid_arg;
	}

	if (bRead == 0) {
		return E_invalid_arg;
	}

	*bRead = 0;

	do {

#if defined(_WIN32)

		if (!ReadFile(fd, cbuf, toRead, LPDWORD(&n), 0)) {
			retval = E_read_failed;
			if (oserr) *oserr = GET_ERRNO;
			break;
		} else if (n == 0) {
			break;
		}

#else

		n = ::read(fd, cbuf, toRead);
		if (n < 0) {
			if (EINTR != GET_ERRNO) {
				retval = E_read_failed;
				if (oserr) *oserr = GET_ERRNO;
				break;
			} else {
				continue;
			}
		} else if (n == 0) {
			break;
		}

#endif

		cbuf += n;
		toRead -= n;
		nbytes += n;
	} while (toRead > 0);

	*bRead = nbytes;
	return retval;
}

/**
 * Reads from the file starting at the specified
 * offset. It first seeks to the specified offset
 * and then reads the data.
 *
 * @param [in]  offset - Starting read offset.
 * @param [out] buf    - Buffer to read the data into.
 * @param [in]  toRead - Number of bytes to read.
 * @param [out] bRead  - Number of bytes read. This
 *                       can be less than toRead.
 * @param [out] oserr  - OS error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
File::read(int64_t offset, void *buf, int toRead, int *bRead, int *oserr)
{
	int retval = File::seek(offset, oserr);
	if (retval == E_ok) {
		retval = read(buf, toRead, bRead, oserr);
	}
	return retval;
}

/**
 * Writes to the file.
 *
 * @param [in]  buf    - Buffer to write the data from.
 * @param [in]  toRead - Number of bytes to write.
 * @param [out] bRead  - Number of bytes written.
 * @param [out] oserr  - OS error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
File::write(const void *buf, int toWrite, int *bWritten, int *oserr)
{
	int retval = E_ok;
	int nbytes = 0;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (buf == 0) {
		return E_invalid_arg;
	}

	if (toWrite <= 0) {
		return E_invalid_arg;
	}

	if (bWritten == 0) {
		return E_invalid_arg;
	}

	*bWritten = 0;

#if defined(_WIN32)

	if (!WriteFile(fd, buf, toWrite, LPDWORD(&nbytes), 0)) {
		nbytes = -1;
	}

#else

	nbytes = ::write(fd, buf, toWrite);

#endif

	if (nbytes < 0) {
		retval = E_write_failed;
		if (oserr) *oserr = GET_ERRNO;
	} else {
		*bWritten = nbytes;
	}

	return retval;
}

/**
 * Writes to the file starting at the specified
 * offset. It first seeks to the specified offset
 * and then writes the data.
 *
 * @param [in]  offset - Starting read offset.
 * @param [in]  buf    - Buffer to write the data from.
 * @param [in]  toRead - Number of bytes to write.
 * @param [out] bRead  - Number of bytes written.
 * @param [out] oserr  - OS error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
File::write(int64_t offset, const void *buf, int toWrite, int *bWritten, int *oserr)
{
	int retval = File::seek(offset, oserr);
	if (retval == E_ok) {
		retval = write(buf, toWrite, bWritten, oserr);
	}
	return retval;
}

/**
 * Seeks to the specified offset in the file.
 *
 * @param [in] whence     - Whence directive. This is same
 *                          as whence in lseek(2) system call.
 * @param [in] offset     - File offset to seek to as per
 *                          the whence directive.
 * @param [out] newOffset - New file offset.
 * @param [out] oserr     - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
File::seek(int whence, int64_t offset, int64_t *newOffset, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (offset < 0) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	DWORD         type;
	LARGE_INTEGER reqOff;
	LARGE_INTEGER newOff;

	switch (whence) {
		case SEEK_SET:
			type = FILE_BEGIN;
			break;

		case SEEK_CUR:
			type = FILE_CURRENT;
			break;

		case SEEK_END:
			type = FILE_END;
			break;

		default:
			return E_invalid_arg;
	}

	reqOff.QuadPart = offset;

	if (!SetFilePointerEx(fd, reqOff, &newOff, type)) {
		retval = E_seek_failed;
		if (oserr) *oserr = GET_ERRNO;
	} else if (newOffset) {
		*newOffset = newOff.QuadPart;
	}
	
#else

	off_t newOff = ::lseek(fd, offset, whence);
	if (newOff == (off_t)-1) {
		retval = E_seek_failed;
		if (oserr) *oserr = GET_ERRNO;
	} else if (newOffset) {
		*newOffset = newOff;
	}

#endif

	return retval;
}

/**
 * Seeks to the specified absolute offset.
 *
 * @param [in] offset - Absolute file offset desired.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
File::seek(int64_t offset, int *oserr)
{
	int     retval = E_ok;
	int64_t newOffset = -1L;

	retval = seek(SEEK_SET, offset, &newOffset, oserr);
	if (retval == E_ok) {
		if (offset != newOffset) {
			retval = E_seek_failed;
		}
	}

	return retval;
}

/**
 * Syncs the file (all the file data is persisted) to the
 * hard disk.
 *
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
File::sync(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

#if defined(_WIN32)

	if (!FlushFileBuffers(fd)) {
		retval = E_sync_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

#else

	if (fsync(fd) < 0) {
		retval = E_sync_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

#endif

	return retval;
}

/**
 * Get the file size.
 *
 * @param [out] oserr - OS error code.
 *
 * @return +ve file size on success, -ve error code
 * on failure.
 */
int64_t
File::size(int *oserr)
{
	int64_t	fsize = 0;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

#if defined(_WIN32)

	LARGE_INTEGER fileSize;

	if (!GetFileSizeEx(fd, &fileSize)) {
		fsize = E_stat_failed;
		if (oserr) *oserr = GET_ERRNO;
	} else {
		fsize = fileSize.QuadPart;
	}

#else

	struct stat statBuf;

	if (fstat(fd, &statBuf) < 0) {
		fsize = E_stat_failed;
		if (oserr) *oserr = GET_ERRNO;
	} else {
		fsize = statBuf.st_size;
	}

#endif

	return fsize;
}

/**
 * Truncates the file to the specified size.
 *
 * @param [in]  fsize - New file size.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
File::truncate(int64_t fsize, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	retval = seek(fsize, oserr);
	if (retval == E_ok) {

#if defined(_WIN32)

		if (!SetEndOfFile(fd)) {
			retval = -1;
		}

#else

		retval = ::ftruncate(fd, fsize);

#endif

		if (retval < 0) {
			retval = E_trunc_failed;
			if (oserr) *oserr = GET_ERRNO;
		}
	}

	return retval;
}

/**
 * Locks a region/section of the file. If the lock is already acquired by
 * another process, the call blocks until the other process release the lock.
 *
 * @param [in] type   - Lock type: LOCK_SHARED/LOCK_EXCLUSIVE
 * @param [in] start  - Starting offset of the region from the start of the file.
 * @param [in] len    - Length of the region in bytes.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success (after acquiring the lock), -ve error code on failure.
 */
int
File::lock(int type, int64_t start, int64_t len, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (start < 0) {
		return E_invalid_arg;
	}

	if (len < 0) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	DWORD           lckflags = LOCK_SHARED;
	LARGE_INTEGER   theStart;
	LARGE_INTEGER   theLen;
	DWORD           low;
	DWORD           high;
	OVERLAPPED      overlapped;

	if (type == LOCK_EXCLUSIVE)
		lckflags |= LOCKFILE_EXCLUSIVE_LOCK;

	theStart.QuadPart = start;
	low = theStart.LowPart;
	high = theStart.HighPart;

	theLen.QuadPart = len;
	overlapped.Offset = theLen.LowPart;
	overlapped.OffsetHigh = theLen.HighPart;
	overlapped.hEvent = 0;

	if (!LockFileEx(fd, lckflags, 0, low, high, &overlapped)) {
		retval = E_lock_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

#else

	struct flock lck;

	if (type == LOCK_SHARED)
	{
		lck.l_type = F_RDLCK;
	}
	else if (type == LOCK_EXCLUSIVE)
	{
		lck.l_type = F_WRLCK;
	}
	else
	{
		return E_invalid_arg;
	}

	lck.l_whence = SEEK_SET;
	lck.l_start = (off_t)start;
	lck.l_len = (off_t)len;

	do {
		if (fcntl(fd, F_SETLKW, &lck) < 0) {
			if (EINTR == GET_ERRNO) {
				continue;
			} else {
				retval = E_lock_failed;
				if (oserr) *oserr = GET_ERRNO;
			}
		} else {
			retval = E_ok;
		}

		break;
	} while (1);

#endif

	return retval;
}

/**
 * Tries to lock a region/secton of the file. If the lock is already acquired
 * by another process, the call returns immediately with the error code
 * E_try_again.
 *
 * @param [in]  type  - Lock type: LOCK_SHARED/LOCK_EXCLUSIVE
 * @param [in]  start - Starting offset of the region from the start of the file.
 * @param [in]  len   - Length of the region in bytes.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success (after acquiring the lock), E_try_again if the lock
 * is acquired by another process, and -ve error code on failure.
 */
int
File::trylock(int type, int64_t start, int64_t len, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (start < 0) {
		return E_invalid_arg;
	}

	if (len < 0) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	DWORD           lckflags = LOCK_SHARED;
	LARGE_INTEGER   theStart;
	LARGE_INTEGER   theLen;
	DWORD           low;
	DWORD           high;
	OVERLAPPED      overlapped;

	type = LOCKFILE_FAIL_IMMEDIATELY;
	if (type == LOCK_EXCLUSIVE)
		lckflags |= LOCKFILE_EXCLUSIVE_LOCK;

	theStart.QuadPart = start;
	low = theStart.LowPart;
	high = theStart.HighPart;

	theLen.QuadPart = len;
	overlapped.Offset = theLen.LowPart;
	overlapped.OffsetHigh = theLen.HighPart;
	overlapped.hEvent = 0;

	if (!LockFileEx(fd, lckflags, 0, low, high, &overlapped)) {
		int error = GET_ERRNO;

		if (ERROR_IO_PENDING == error) {
			retval = E_try_again;
		} else {
			retval = E_lock_failed;
		}

		if (oserr) *oserr = error;
	}

#else

	struct flock lck;

	if (type == LOCK_SHARED)
	{
		lck.l_type = F_RDLCK;
	}
	else if (type == LOCK_EXCLUSIVE)
	{
		lck.l_type = F_WRLCK;
	}
	else
	{
		return E_invalid_arg;
	}

	lck.l_whence = SEEK_SET;
	lck.l_start = (off_t)start;
	lck.l_len = (off_t)len;

	do {
		if (fcntl(fd, F_SETLK, &lck) < 0) {
			int error = GET_ERRNO;

			if (EINTR == error) {
				continue;
			} else if ((EACCES == error) || (EAGAIN == error)) {
				retval = E_try_again;
			} else {
				retval = E_lock_failed;
			}

			if (oserr) *oserr = error;
		} else {
			retval = E_ok;
		}

		break;
	} while (1);

#endif

	return retval;
}

/**
 * Unlocks a region/section of the file.
 *
 * @param [in]  start - Starting offset of the region from the start of the file.
 * @param [in]  len   - Length of the region in bytes.
 * @param [out] oserr - OS error code.
 *
 * @return E_ok on success (after releasing the lock), -ve error code on failure.
 */
int
File::unlock(int64_t start, int64_t len, int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (fd == INVALID_HANDLE_VALUE) {
		return E_invalid_state;
	}

	if (start < 0) {
		return E_invalid_arg;
	}

	if (len < 0) {
		return E_invalid_arg;
	}

#if defined(_WIN32)

	LARGE_INTEGER   theStart;
	LARGE_INTEGER   theLen;
	DWORD           low;
	DWORD           high;
	OVERLAPPED      overlapped;

	theStart.QuadPart = start;
	low = theStart.LowPart;
	high = theStart.HighPart;

	theLen.QuadPart = len;
	overlapped.Offset = theLen.LowPart;
	overlapped.OffsetHigh = theLen.HighPart;
	overlapped.hEvent = 0;

	if (!UnlockFileEx(fd, 0, low, high, &overlapped)) {
		retval = E_unlock_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

#else

	struct flock lck;

	lck.l_type = F_UNLCK;
	lck.l_whence = SEEK_SET;
	lck.l_start = (off_t)start;
	lck.l_len = (off_t)len;

	if (fcntl(fd, F_SETLK, &lck) < 0) {
		retval = E_unlock_failed;
		if (oserr) *oserr = GET_ERRNO;
	}

#endif

	return retval;
}

/**
 * Closes the file it opened.
 *
 * @param [out] oserr - OS error code.
 * @return E_ok on success, -ve error code on failure.
 */
int
File::close(int *oserr)
{
	if (fd != INVALID_HANDLE_VALUE) {
#if defined(_WIN32)
		if (!CloseHandle(fd)) {
			if (oserr) *oserr = GET_ERRNO;
			return E_close_failed;
		}
#else
		if (::close(fd) < 0) {
			if (oserr) *oserr = GET_ERRNO;
			return E_close_failed;
		}
#endif
		fd = INVALID_HANDLE_VALUE;
	}

	return E_ok;
}
