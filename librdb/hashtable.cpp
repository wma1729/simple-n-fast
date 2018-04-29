#if !defined(_WIN32)
#include <sys/mman.h>
#endif

#include "rdb/hashtable.h"

/*
 * Initializes hash table entry.
 *
 * @param [out] hent - Hash table entry to initialize.
 */
void
HashTable::initHashEntry(hash_entry_t *hent)
{
	Assert((hent != 0), __FILE__, __LINE__,
		"invalid hash entry");

	hent->offset = -1L;
	hent->head = 0;
	hent->tail = 0;
	hent->rwlock = 0;
}

/**
 * Allocates and initializes the hash table.
 *
 * @param [in] size - Hash table size.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
HashTable::allocate(int size)
{
	const char	*who = "HashTable::allocate";
	size_t		len = size * sizeof(hash_entry_t);

	ht = (hash_entry_t *)malloc(len);
	if (ht == 0) {
		Log(ERR, who, errno,
			"failed to allocate memory for hash table");
		return E_no_memory;
	} else {
#if !defined(_WIN32)
		posix_madvise(ht, len, MADV_WILLNEED);
#endif
		for (int i = 0; i < size; ++i)
			initHashEntry(ht + i);

		htsize = size;
		return E_ok;
	}
}

/**
 * Acquires read lock on hash table entry at the
 * specified index.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::rdlock(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	MutexGuard guard(mutex);

	if (hent->rwlock == 0) {
		hent->rwlock = rwlockPool->get();
	}

	Assert((hent->rwlock != 0), __FILE__, __LINE__,
		"failed to get read write lock");

	int error = 0;
	int r = hent->rwlock->rdlock(&error);
	Assert((r == E_ok), __FILE__, __LINE__, error,
		"failed to get read lock on %d", index);
}

/**
 * Releases read lock on hash table entry at the
 * specified index.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::rdunlock(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	MutexGuard guard(mutex);

	if (hent->rwlock) {
		hent->rwlock->rdunlock();
		if (hent->rwlock->getLockCount() == 0) {
			rwlockPool->put(hent->rwlock);
			hent->rwlock = 0;
		}
	}
}

/**
 * Acquires write lock on hash table entry at the
 * specified index.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::wrlock(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	MutexGuard guard(mutex);

	if (hent->rwlock == 0) {
		hent->rwlock = rwlockPool->get();
	}

	Assert((hent->rwlock != 0), __FILE__, __LINE__,
		"failed to get read write lock");

	int error = 0;
	int r = hent->rwlock->wrlock(&error);
	Assert((r == E_ok), __FILE__, __LINE__, error,
		"failed to get write lock on %d", index);
}

/**
 * Releases lock on hash table entry at the
 * specified index.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::wrunlock(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	MutexGuard guard(mutex);

	if (hent->rwlock) {
		hent->rwlock->wrunlock();
		rwlockPool->put(hent->rwlock);
		hent->rwlock = 0;
	}
}

/**
 * Gets the first key page offset at the specified index.
 *
 * @param [in] index - Hash table entry index.
 *
 * @return +ve key page offset. May be -1 if the hash entry
 * is unused.
 */
int64_t
HashTable::getOffset(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	return hent->offset;
}

/**
 * Sets the first key page offset at the specified index.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::setOffset(int index, int64_t offset)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	hent->offset = offset;
}

/**
 * Adds the key page node to the end of the list.
 *
 * @param [in] index - Hash table entry index.
 * @param [in] kpn   - Key page node.
 *
 * @return Always E_ok. May change in future.
 */
int
HashTable::addKeyPageNode(int index, key_page_node_t *kpn)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	kpn->kpn_prev = kpn->kpn_next = 0;

	hash_entry_t *hent = ht + index;

	if (hent->head == 0) {
		hent->head = kpn;
		hent->tail = kpn;
		hent->offset = kpn->kpn_kpoff;
	} else {
		kpn->kpn_prev = hent->tail;
		hent->tail->kpn_next = kpn;
		hent->tail = kpn;
	}

	return E_ok;
}

/**
 * Removes the given key page node from the list.
 *
 * @param [in] index - Hash table entry index.
 * @param [in] kpn   - Key page node to remove.
 */
void
HashTable::removeKeyPageNode(int index, key_page_node_t *kpn)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;

	Assert(((hent->head != 0) && (hent->tail != 0)), __FILE__, __LINE__,
		"key page list is empty");

	if (kpn->kpn_prev) {
		kpn->kpn_prev->kpn_next = kpn->kpn_next;
	} else {
		hent->head = kpn->kpn_next;
		if (hent->head) {
			hent->offset = kpn->kpn_next->kpn_kpoff;
		} else {
			hent->offset = -1L;
		}
	}

	if (kpn->kpn_next) {
		kpn->kpn_next->kpn_prev = kpn->kpn_prev;
	} else {
		hent->tail = kpn->kpn_prev;
	}
}

/**
 * Gets the head of the key page list.
 *
 * @param [in] index - Hash table entry index.
 *
 * @return head of the key page list.
 */
key_page_node_t *
HashTable::getKeyPageNodeList(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;
	return hent->head;
}

/**
 * Frees key page node list for the hash entry.
 *
 * @param [in] index - Hash table entry index.
 */
void
HashTable::freeKeyPageNodeList(int index)
{
	Assert(((index >= 0) && (index < htsize)), __FILE__, __LINE__,
		"out-of-bound hash table index (%d), range [%d, %d)",
		index, 0, htsize);

	hash_entry_t *hent = ht + index;
	if (hent->head) {
		key_page_node_t *kpn;
		while ((kpn = hent->head) != 0) {
			hent->head = hent->head->kpn_next;
			::free(kpn);
		}
	}
}
