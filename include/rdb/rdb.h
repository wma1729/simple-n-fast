#ifndef _SNF_RDB_RDB_H_
#define _SNF_RDB_RDB_H_

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

class Rdb
{
private:
	std::string path;
	std::string name;
	int         kpSize;
	int         htSize;
	HashTable   *hashTable;
	KeyFile     *keyFile;
	ValueFile   *valueFile;
	LRUCache    *cache;

	int populateHashTable();
	int populateFreePages(const char *);
	int addNewPage(key_info_t *);
	int processKeyPages(key_info_t *, op_t);

public:
	Rdb(const std::string &path,
		const std::string &name)
	{
		this->path = path;
		this->name = name;
		this->kpSize = KP_SIZE;
		this->htSize = NextPrime(HASH_TABLE_SIZE);
		this->hashTable = 0;
		this->keyFile = 0;
		this->valueFile = 0;
		this->cache = 0;
	}

	Rdb(const std::string &path,
		const std::string &name,
		int kpSize,
		int htSize)
	{
		Assert((kpSize > KEY_PAGE_HDR_SIZE), __FILE__, __LINE__,
			"invalid page size (%d); should at least be > %d",
			kpSize, KEY_PAGE_HDR_SIZE);

		Assert(((kpSize % KEY_PAGE_HDR_SIZE) == 0), __FILE__, __LINE__,
			"page size (%d) is not a multiple of %d",
			kpSize, KEY_PAGE_HDR_SIZE);

		Assert((htSize > 0), __FILE__, __LINE__,
			"invalid hash table size (%d)", htSize);

		this->path = path;
		this->name = name;
		this->kpSize = kpSize;
		this->htSize = NextPrime(htSize);
		this->hashTable = 0;
		this->keyFile = 0;
		this->valueFile = 0;
		this->cache = 0;
	}

	~Rdb()
	{
		close();
	}

	const char *getPath() const
	{
		return path.c_str();
	}

	const char *getName() const
	{
		return name.c_str();
	}

	int getKeyPageSize() const
	{
		return kpSize;
	}

	void setKeyPageSize(int);

	int getHashTableSize() const
	{
		return htSize;
	}

	void setHashTableSize(int);

	int open();
	int get(const char *key, int klen, char *value, int *vlen);
	int set(const char *key, int klen, const char *value, int vlen);
	int remove(const char *key, int klen);
	void close();
};

#endif // _SNF_RDB_RDB_H_
