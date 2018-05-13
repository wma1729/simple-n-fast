#ifndef _SNF_RDB_HASHTABLE_H_
#define _SNF_RDB_HASHTABLE_H_

#include <mutex>
#include "dbstruct.h"
#include "rwlock.h"

#ifndef HASH_TABLE_SIZE
#define HASH_TABLE_SIZE 500000
#endif

/*
 * Hash function. I think this is the same one as used
 * by sdbm.
 *
 * @param key - the key whose hash value is evaluated.
 * @param klen - the key length.
 * @param table_size - the hash table size.
 *
 * @return the hash value of the key.
 */
inline int
hash(const char *key, int klen, int table_size)
{
	unsigned long ih = 0;

	for (int i = 0; i < klen; i++)
		ih = key[i] + (ih << 6) + (ih << 16) - ih;

	return (int)(ih % table_size);
}

/*
 * Entry of the hash table (32-byte long). The size of
 * the entire hash table is:
 * <number_of_entries> * sizeof(hash_entry_t)
 */
extern "C"
typedef struct hash_entry
{
	int64_t         offset;   // 8, offset of the first page on disk
	key_page_node_t *head;    // 8, pointer to the first page in memory
	key_page_node_t *tail;    // 8, pointer to the last page in memory
	RWLock          *rwlock;  // 8, read-write lock (set on-demand)
} hash_entry_t;

/**
 * The main hash table.
 */
class HashTable
{
private:
	hash_entry_t    *ht;
	int             htsize;
	std::mutex      mutex;
	RWLockPool      *rwlockPool;

	void initHashEntry(hash_entry_t *);

public:
	/**
	 * Constructs the hash table object.
	 */
	HashTable()
		: ht(0),
		  htsize(0),
		  rwlockPool(DBG_NEW RWLockPool())
	{
	}

	/**
	 * Destroys the hash table object.
	 */
	~HashTable()
	{
		if (ht && htsize) {
			for (int i = 0; i < htsize; ++i) {
				freeKeyPageNodeList(i);
			}
			::free(ht);
			ht = 0;
			htsize = 0;
		}

		delete rwlockPool;
	}

	int size() const
	{
		return htsize;
	}

	int allocate(int);
	void rdlock(int);
	void rdunlock(int);
	void wrlock(int);
	void wrunlock(int);

	int64_t getOffset(int);
	void setOffset(int, int64_t);

	int addKeyPageNode(int, key_page_node_t *);
	void removeKeyPageNode(int, key_page_node_t *);
	key_page_node_t *getKeyPageNodeList(int);
	void freeKeyPageNodeList(int);
};

class HTLockGuard
{
private:
	HashTable   *hashTable;
	int         index;
	bool        exclusive;

public:
	HTLockGuard(HashTable *ht, int i, bool excl)
		: hashTable(ht),
		  index(i),
		  exclusive(excl)
	{
		Assert((hashTable != 0), __FILE__, __LINE__,
			"invalid hash table");

		if (exclusive)
			hashTable->wrlock(index);
		else
			hashTable->rdlock(index);
	}

	~HTLockGuard()
	{
		if (exclusive)
			hashTable->wrunlock(index);
		else
			hashTable->rdunlock(index);
	}
};

#endif // _SNF_RDB_HASHTABLE_H_
