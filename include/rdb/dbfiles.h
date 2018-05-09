#ifndef _SNF_RDB_DBFILES_H_
#define _SNF_RDB_DBFILES_H_

#include <mutex>
#include "file.h"
#include "rdb/dbstruct.h"
#include "rdb/fdpmgr.h"

/**
 * Manage DB attributes file.
 */
class AttrFile : public File
{
private:
	dbattr_t    dbAttr;

public:
	/**
	 * Constructs attributes file manager object.
	 *
	 * @param [in] fname - file name
	 * @param [in] mask  - umask to use when opening
	 *                     the file.
	 */
	AttrFile(const char *fname, mode_t mask)
		: File(fname, mask)
	{
		memset(&dbAttr, 0, sizeof(dbattr_t));
	}

	/**
	 * Destroys attributes file manager object.
	 */
	~AttrFile()
	{
	}

	int getKeyPageSize() const
	{
		return dbAttr.a_kpsize;
	}

	void setKeyPageSize(int kpSize)
	{
		dbAttr.a_kpsize = kpSize;
	}

	int getHashTableSize() const
	{
		return dbAttr.a_htsize;
	}

	void setHashTableSize(int htSize)
	{
		dbAttr.a_htsize = htSize;
	}

	int open();
	int read();
	int write();
};

/**
 * Manages key file.
 */
class KeyFile : public File
{
private:
	FreeDiskPageMgr *fdpMgr;
	std::mutex      mutex;

public:
	/**
	 * Constructs key file object.
	 *
	 * @param [in] fname - file name
	 * @param [in] mask  - umask to use when opening
	 *                     the file.
	 */
	KeyFile(const char *fname, mode_t mask)
		: File(fname, mask),
		  fdpMgr(0)
	{
	}

	/**
	 * Destroys value file manager object.
	 */
	~KeyFile()
	{
		if (fdpMgr) {
			delete fdpMgr;
			fdpMgr = 0;
		}
	}

	/**
	 * Gets free disk page manager associated with
	 * the key file manager.
	 *
	 * @return the free disk page manager or NULL
	 * if it is not set yet.
	 */
	FreeDiskPageMgr *getFreeDiskPageMgr()
	{
		return fdpMgr;
	}

	/**
	 * Sets free disk page manager.
	 *
	 * @param [in] fdpMgr - free disk page manager.
	 */
	void setFreeDiskPageMgr(FreeDiskPageMgr *fdpMgr)
	{
		this->fdpMgr = fdpMgr;
	}

	int open(bool);
	int read(int64_t, void *, int);
	int write(int64_t, const void *, int);
	int write(int64_t *, const void *, int);
	int writeFlags(int64_t, key_page_t *, int);
	int writePrevOffset(int64_t, key_page_t *, int64_t);
	int writeNextOffset(int64_t, key_page_t *, int64_t);
	int freePage(int64_t);
};

/**
 * Manages value file.
 */
class ValueFile : public File
{
private:
	FreeDiskPageMgr *fdpMgr;
	std::mutex      mutex;

public:
	/**
	 * Constructs value file manager object.
	 *
	 * @param [in] fname - file name
	 * @param [in] mask  - umask to use when opening
	 *                     the file.
	 */
	ValueFile(const char *fname, mode_t mask)
		: File(fname, mask)
	{
		this->fdpMgr = 0;
	}

	/**
	 * Destroys value file manager object.
	 */
	~ValueFile()
	{
		if (fdpMgr) {
			delete fdpMgr;
			fdpMgr = 0;
		}
	}

	/**
	 * Gets free disk page manager associated with
	 * the value file manager.
	 *
	 * @return the free disk page manager or NULL
	 * if it is not set yet.
	 */
	FreeDiskPageMgr *getFreeDiskPageMgr()
	{
		return fdpMgr;
	}

	/**
	 * Sets free disk page manager.
	 *
	 * @param [in] fdpMgr - free disk page manager.
	 */
	void setFreeDiskPageMgr(FreeDiskPageMgr *fdpMgr)
	{
		this->fdpMgr = fdpMgr;
	}

	int open(bool);
	int read(int64_t, value_page_t *);
	int readFlags(int64_t, int *);
	int write(int64_t, const value_page_t *);
	int write(int64_t *, const value_page_t *);
	int writeFlags(int64_t, value_page_t *, int);
	int freePage(int64_t);
};

#endif // _SNF_RDB_DBFILES_H_
