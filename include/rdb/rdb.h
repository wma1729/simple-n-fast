#ifndef _SNF_RDB_RDB_H_
#define _SNF_RDB_RDB_H_

#include "error.h"
#include "rdb/cache.h"
#include "rdb/dbfiles.h"
#include "rdb/hashtable.h"

int NextPrime(int); // from librdb/prime.cpp

typedef enum op {
	NIL,
	GET,
	SET,
	DEL
} op_t;

/**
 * RDB options set while creating the \em Rdb object
 */
class RdbOptions
{
private:
	int         o_memusage;     // memory usage for key pages in %
	bool        o_syncdata;     // always sync db file
	bool        o_syncidx;      // always sync index file

public:
	/**
	 * Initialize the default Rdb options.
	 */
	RdbOptions()
	{
		o_memusage = 75;
		o_syncdata = true;
		o_syncidx = false;
	}

	/**
	 * Initialize the Rdb options from another RdbOptions object.
	 */
	RdbOptions(const RdbOptions &opt)
	{
		o_memusage = opt.o_memusage;
		o_syncdata = opt.o_syncdata;
		o_syncidx = opt.o_syncidx;
	}

	/**
	 * Get the memory to be used, in % of total RAM size, for
	 * key pages.
	 */
	int getMemoryUsage() const
	{
		return o_memusage;
	}

	/**
	 * Sets the memory to be used, in % of total RAM size, for
	 * key pages.
	 *
	 * @param [in] memusage - desired memory usage for key pages.
	 *
	 * @return E_ok on success, -ve error code on failure.
	 */
	int setMemoryUsage(int memusage)
	{
		const char  *caller = "setMemoryUsage";

		if ((memusage < 5) || (memusage > 80)) {
			Log(ERR, caller,
				"invalid memory usage (%d); should be in the range [5, 80]",
				memusage);
			return E_invalid_arg;
		}

		o_memusage = memusage;
		return E_ok;
	}

	/**
	 * Should the DB data file be synced on every write?
	 */
	bool syncDataFile() const
	{
		return o_syncdata;
	}

	/**
	 * Sets the sync behaviour for DB data file.
	 */
	void syncDataFile(bool syncdb)
	{
		o_syncdata = syncdb;
	}

	/**
	 * Should the DB index file be synced on every write?
	 */
	bool syncIndexFile() const
	{
		return o_syncidx;
	}

	/**
	 * Sets the sync behaviour for DB index file.
	 */
	void syncIndexFile(bool syncidx)
	{
		o_syncidx = syncidx;
	}

	/**
	 * Copy operator.
	 */
	RdbOptions & operator= (const RdbOptions &opt)
	{
		if (this != &opt) {
			o_memusage = opt.o_memusage;
			o_syncdata = opt.o_syncdata;
			o_syncidx = opt.o_syncidx;
		}

		return *this;
	}
};

/**
 * The main database class.
 */
class Rdb
{
private:
	std::string path;
	std::string name;
	int         kpSize;
	int         htSize;
	RdbOptions  options;
	HashTable   *hashTable;
	KeyFile     *keyFile;
	ValueFile   *valueFile;
	LRUCache    *cache;
	bool        opened;
	Mutex       openMutex;
	int         opCount;
	Mutex       opMutex;

	inline void init(
		const std::string &path,
		const std::string &name,
		int kpsize,
		int htsize)
	{
		Assert((kpsize > MIN_KEY_PAGE_SIZE), __FILE__, __LINE__,
			"invalid page size (%d); should at least be %d",
			kpsize, MIN_KEY_PAGE_SIZE);

		Assert(((kpsize % KEY_PAGE_HDR_SIZE) == 0), __FILE__, __LINE__,
			"page size (%d) is not a multiple of %d",
			kpsize, KEY_PAGE_HDR_SIZE);

		Assert((htSize > 0), __FILE__, __LINE__,
			"invalid hash table size (%d)", htSize);

		this->path = path;
		this->name = name;
		this->kpSize = kpsize;
		this->htSize = NextPrime(htsize);
		this->hashTable = 0;
		this->keyFile = 0;
		this->valueFile = 0;
		this->cache = 0;
		this->opened = false;
		this->opCount = 0;
	}

	int populateHashTable();
	int populateFreePages(const char *);
	int addNewPage(key_info_t *);
	int processKeyPages(key_info_t *, op_t);
	int backupFile(const char *);
	int restoreFile(const char *);
	int removeBackupFile(const char *);

public:
	/**
	 * Constructs the Rdb object. The default values are
	 * used for:
	 * - key page size
	 * - hash table size
	 * - options
	 *
	 * @param [in] path - database path.
	 * @param [in] name - database name.
	 */
	Rdb(const std::string &path,
		const std::string &name)
		: options(),
		  openMutex(),
		  opMutex()
	{
		init(path, name, KEY_PAGE_SIZE, HASH_TABLE_SIZE);
	}

	/**
	 * Constructs the Rdb object. The default values are used
	 * for:
	 * - key page size
	 * - hash table size
	 *
	 * @param [in] path    - database path.
	 * @param [in] name    - database name.
	 * @param [in] options - database options.
	 */
	Rdb(const std::string &path,
		const std::string &name,
		const RdbOptions &opt)
		: options(opt),
		  openMutex(),
		  opMutex()
	{
		init(path, name, KEY_PAGE_SIZE, HASH_TABLE_SIZE);
	}

	/**
	 * Constructs the Rdb object. The default values are
	 * used for:
	 * - options
	 *
	 * @param [in] path   - database path.
	 * @param [in] name   - database name.
	 * @param [in] kpsize - key page size.
	 * @param [in] htsize - hash table size.
	 */
	Rdb(const std::string &path,
		const std::string &name,
		int kpsize,
		int htsize)
		: options(),
		  openMutex(),
		  opMutex()
	{
		init(path, name, kpsize, htsize);
	}

	/**
	 * Constructs the Rdb object.
	 *
	 * @param [in] path    - database path.
	 * @param [in] name    - database name.
	 * @param [in] kpsize  - key page size.
	 * @param [in] htsize  - hash table size.
	 * @param [in] options - database options.
	 */
	Rdb(const std::string &path,
		const std::string &name,
		int kpsize,
		int htsize,
		const RdbOptions &opt)
		: options(opt),
		  openMutex(),
		  opMutex()
	{
		init(path, name, kpsize, htsize);
	}

	~Rdb()
	{
		close();
	}

	/**
	 * Gets the database path.
	 */
	const char *getPath() const
	{
		return path.c_str();
	}

	/**
	 * Gets the database name.
	 */
	const char *getName() const
	{
		return name.c_str();
	}

	/**
	 * Gets the key page size.
	 */
	int getKeyPageSize() const
	{
		return kpSize;
	}

	/**
	 * Sets the key page size. Must be called before opening
	 * the database for the first time (time of database
	 * creation).
	 *
	 * @param [in] kpsize - desired key page size.
	 *
	 * @return E_ok on success, -ve error code on failure.
	 */
	int setKeyPageSize(int kpsize)
	{
		const char  *caller = "setKetPageSize";

		if (kpsize < MIN_KEY_PAGE_SIZE) {
			Log(ERR, caller, "invalid page size (%d); should at least be %d",
				kpsize, MIN_KEY_PAGE_SIZE);
			return E_invalid_arg;
		}

		if ((kpsize % KEY_PAGE_HDR_SIZE) != 0) {
			Log(ERR, caller, "page size (%d) is not a multiple of %d",
				kpsize, KEY_PAGE_HDR_SIZE);
			return E_invalid_arg;
		}

		this->kpSize = kpsize;
		return E_ok;
	}

	/**
	 * Gets the hash table size.
	 */
	int getHashTableSize() const
	{
		return htSize;
	}

	/**
	 * Sets the hash table size. Must be called before opening
	 * the database for the first time (time of database
	 * creation).
	 *
	 * @param [in] htsize - desired hash table size.
	 *
	 * @return E_ok on success, -ve error code on failure.
	 */
	int setHashTableSize(int htsize)
	{
		const char  *caller = "setHashTableSize";

		if (htsize <= 0) {
			Log(ERR, caller,
				"invalid hash table size (%d)", htSize);
			return E_invalid_arg;
		}

		this->htSize = NextPrime(htSize);
		return E_ok;
	}

	int open();
	int get(const char *key, int klen, char *value, int *vlen);
	int set(const char *key, int klen, const char *value, int vlen);
	int remove(const char *key, int klen);
	int rebuild();
	int close();
};

#endif // _SNF_RDB_RDB_H_
