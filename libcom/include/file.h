#ifndef _SNF_FILE_H_
#define _SNF_FILE_H_

#include "common.h"
#if !defined(_WIN32)
	#include <sys/types.h>
	#include <sys/stat.h>
#endif

namespace snf {

/**
 * A simple wrapper around FILE object.
 */
class file_ptr
{
private:
	FILE            *m_fp;
	std::string     m_name;

	void init(const char *n, const char *a)
	{
		m_fp = fopen(n, a);
		if (m_fp == nullptr) {
			std::ostringstream oss;
			oss << "failed to open key file " << n;
			throw std::system_error(
				snf::system_error(),
				std::system_category(),
				oss.str());
		}
	}

public:
	file_ptr(const char *n, const char *a) : m_name(n) { init(n, a); }
	file_ptr(const std::string &n, const char *a) : m_name(n) { init(n.c_str(), a); }
	explicit file_ptr(FILE *fp) : m_fp(fp) { }
	~file_ptr() { if (m_fp) fclose(m_fp); }
	const char *name() const { return m_name.c_str(); }
	operator FILE* () { return m_fp; }

};

/**
 * A simple class to manage file operations.
 */
class file
{
protected:
	std::string	fname;
	mode_t		mask;
	fhandle_t	fd;

public:
	enum class lock_type { shared, exclusive };

	/**
	 * File Open mode
	 */
	struct open_flags
	{
		bool	o_read;
		bool	o_write;
		bool	o_append;
		bool	o_create;
		bool	o_truncate;
		bool	o_excl;
		bool	o_sync;

		open_flags()
		{
			o_read = false;
			o_write = false;
			o_append = false;
			o_create = false;
			o_truncate = false;
			o_excl = false;
			o_sync = false;
		}
	};

	/**
	 * Constructs the file manager object using the
	 * file name and the desired umask.
	 *
	 * @param [in] fname - file name
	 * @param [in] mask  - umask to use when opening
	 *                     the file.
	 */
	file(const std::string &fname, mode_t mask)
	{
		this->fname = fname;
		this->mask = mask;
		this->fd = INVALID_HANDLE_VALUE;
	}

	/**
	 * Destroys the file manager object. The file, if open,
	 * is closed.
	 */
	virtual ~file()
	{
		close();
	}

	/**
	 * Gets the file name that this object is managing.
	 */
	const char *name() const
	{
		return fname.c_str();
	}

	/**
	 * Gets the low level file handle.
	 */
	fhandle_t handle() const
	{
		return fd;
	}

	virtual int     open(const open_flags &, mode_t mode = 0600, int *oserr = 0);
	virtual int     read(void *, int, int *, int *oserr = 0);
	virtual int     read(int64_t, void *, int, int *, int *oserr = 0);
	virtual int     write(const void *, int, int *, int *oserr = 0);
	virtual int     write(int64_t, const void *, int, int *, int *oserr = 0);
	virtual int     seek(int, int64_t, int64_t *newOffset = 0, int *oserr = 0);
	virtual int     seek(int64_t, int *oserr = 0);
	virtual int     sync(int *oserr = 0);
	virtual int64_t size(int *oserr = 0);
	virtual int     truncate(int64_t, int *oserr = 0);
	virtual int     lock(lock_type, int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     trylock(lock_type, int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     unlock(int64_t, int64_t len = 0, int *oserr = 0);
	virtual int     close(int *oserr = 0);
};

} // namespace snf

#endif // _SNF_FILE_H_
