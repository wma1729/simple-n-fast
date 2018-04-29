#ifndef _SNF_FILE_H_
#define _SNF_FILE_H_

#include "common.h"

/**
 * File Open mode
 */
struct FileOpenFlags
{
	bool	o_read;
	bool	o_write;
	bool	o_append;
	bool	o_create;
	bool	o_truncate;
	bool	o_excl;
	bool	o_sync;

	FileOpenFlags()
	{
		o_read = false;
		o_write = false;
		o_append = false;
		o_create = false;
		o_truncate = false;
		o_excl = false;
		o_sync = false;
	}

	FileOpenFlags(const FileOpenFlags &flags)
	{
		if (this != &flags) {
			this->o_read = flags.o_read;
			this->o_write = flags.o_write;
			this->o_append = flags.o_append;
			this->o_create = flags.o_create;
			this->o_truncate = flags.o_truncate;
			this->o_excl = flags.o_excl;
			this->o_sync = flags.o_sync;
		}
	}

	FileOpenFlags & operator= (const FileOpenFlags &flags)
	{
		if (this != &flags) {
			this->o_read = flags.o_read;
			this->o_write = flags.o_write;
			this->o_append = flags.o_append;
			this->o_create = flags.o_create;
			this->o_truncate = flags.o_truncate;
			this->o_excl = flags.o_excl;
			this->o_sync = flags.o_sync;
		}

		return *this;
	}
};

/**
 * File mask guard. Only for Unix platforms. NOOP on windows.
 */
class FileMask
{
#if !defined(_WIN32)
private:
	mode_t	omask;
#endif

public:
	FileMask(mode_t mask)
	{
#if !defined(_WIN32)
		// Set the new mask and save the old mask
		omask = umask(mask);
#endif
	}

	~FileMask()
	{
#if !defined(_WIN32)
		// Restore the old mask
		umask(omask);
#endif
	}
};

/**
 * A simple class to manage file operations.
 */
class File
{
protected:
	std::string	fname;
	mode_t		mask;
	fhandle_t	fd;

public:
	static const int LOCK_SHARED = 0;
	static const int LOCK_EXCLUSIVE = 1;

	/**
	 * Constructs the file manager object using the
	 * file name and the desired umask.
	 *
	 * @param [in] fname - file name
	 * @param [in] mask  - umask to use when opening
	 *                     the file.
	 */
	File(const char *fname, mode_t mask)
	{
		this->fname = fname;
		this->mask = mask;
		this->fd = INVALID_HANDLE_VALUE;
	}

	/**
	 * Destroys the file manager object. The file, if open,
	 * is closed.
	 */
	virtual ~File()
	{
		close();
	}

	/**
	 * Gets the file name that this object is managing.
	 */
	const char *getFileName() const
	{
		return fname.c_str();
	}

	/**
	 * Gets the low level file handle.
	 */
	fhandle_t getFileHandle() const
	{
		return fd;
	}

	virtual int     open(const FileOpenFlags &, mode_t mode = 0600, int *oserr = 0);
	virtual int     read(void *, int, int *, int *oserr = 0);
	virtual int     read(int64_t, void *, int, int *, int *oserr = 0);
	virtual int     write(const void *, int, int *, int *oserr = 0);
	virtual int     write(int64_t, const void *, int, int *, int *oserr = 0);
	virtual int     seek(int, int64_t, int64_t *newOffset = 0, int *oserr = 0);
	virtual int     seek(int64_t, int *oserr = 0);
	virtual int     sync(int *oserr = 0);
	virtual int64_t size(int *oserr = 0);
	virtual int     truncate(int64_t, int *oserr = 0);
	virtual int     lock(int, int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     trylock(int, int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     unlock(int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     close(int *oserr = 0);
};

#endif // _SNF_FILE_H_
