#include <memory>
#include "filesystem.h"
#include "keyrec.h"
#include "rdb.h"
#include "unwind.h"

/*
 * Reads the key file and do the following:
 * 1. Populates the hash table i.e. set the offset of the first
 *    key page in the hash table entry.
 * 2. Prepares the in-memory free disk key page stack.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::populateHashTable()
{
	const char  *caller = "Rdb::populateHashTable";
	int         retval = E_ok;
	int64_t     offset = 0;
	key_page_t  kp;

	Log(DBG, caller, "preparing hash table and free disk pages in index file");

	keyFile->setFreeDiskPageMgr(DBG_NEW FreeDiskPageMgr(kpSize));

	// Set the end of the file as the first free page
	retval = keyFile->freePage(keyFile->size());

	while (retval == E_ok) {
		retval = keyFile->read(offset, &kp, KEY_PAGE_HDR_SIZE);
		if (retval != E_ok) {
			if (retval == E_eof_detected) {
				retval = E_ok;
			}
			break;
		} else {
			if (IsKeyPageDeleted(&kp) || (kp.kp_vcount <= 0)) {
				keyFile->freePage(offset);
			} else {
				if ((kp.kp_poff == -1L) && (hashTable->getOffset(kp.kp_hash) == -1L)) {
					// Set the offset of the first page in the hash table
					hashTable->setOffset(kp.kp_hash, offset);
				}
			}
		}

		offset += kpSize;
	}

	return retval;
}

/*
 * Prepares the free disk db page stack. It relies on
 * <dbname>.fdp file. If it cannot read that file, it
 * reads the whole db file to achieve this.
 *
 * @param [in] fname - <dbname>.fdp file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::populateFreePages(const char *fname)
{
	const char  *caller = "Rdb::populateFreePages";
	int         retval = E_ok;
	int         oserr = 0;
	int         flags;
	snf::file   *file = DBG_NEW snf::file(fname, 0022);
	int64_t     vfsize = -1L;
	int64_t     offset;

	Log(DBG, caller, "preparing free disk pages in db file");

	snf::file::open_flags oflags;
	oflags.o_read = true;
	oflags.o_write = true;
	oflags.o_create = true;

	retval = file->open(oflags, 0600, &oserr);
	if (retval != E_ok) {
		Log(ERR, caller, oserr,
			"failed to open free disk page file %s", fname);
		return retval;
	}

	FreeDiskPageMgr *fdpMgr = DBG_NEW FreeDiskPageMgr(sizeof(value_page_t), file);
	retval = fdpMgr->init();
	if (retval != E_ok) {
		// Something went wrong
		retval = fdpMgr->reset();
		if (retval != E_ok) {
			return retval;
		}

		// Use the hard way to get free pages

		vfsize = valueFile->size();
		offset = 0L;

		// Set the end of the file as the first free page
		retval = fdpMgr->free(vfsize);

		while (retval == E_ok) {
			retval = valueFile->readFlags(offset, &flags);
			if (retval != E_ok) {
				if (retval == E_eof_detected) {
					retval = E_ok;
				}
				break;
			} else {
				if ((flags & VPAGE_DELETED) == VPAGE_DELETED) {
					retval = fdpMgr->free(offset);
				}
			}

			offset += sizeof(value_page_t);
		}
	}

	if (retval == E_ok) {
		if (fdpMgr->size() == 0) {
			// Most likely we are opening the db for the first time
			if (-1L == vfsize) {
				valueFile->size();
			}
			fdpMgr->free(vfsize);
		}
		valueFile->setFreeDiskPageMgr(fdpMgr);
	}

	return retval;
}

/*
 * Main function to process the key pages and find the
 * correct key page that holds (or can hold) the key.
 *
 * @param [inout] key - key information.
 * @param [in]    op  - operation being performed.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::processKeyPages(key_info_t *ki, op_t op)
{
	const char      *caller = "Rdb::processKeyPages";
	int             retval = E_ok;
	key_page_t      *kp = 0;
	int64_t         nextOffset = hashTable->getOffset(ki->ki_hash);
	key_page_node_t *kpn = hashTable->getKeyPageNodeList(ki->ki_hash);

	while ((retval == E_ok) &&
			(nextOffset != -1L) &&
			(kpn != 0)) {

		cache->touch(kpn);

		if (kpn->kpn_kp == 0) {
			retval = cache->update(kpn, nextOffset);
			if (retval != E_ok) {
				Log(ERR, caller,
					"failed to read key page at offset %" PRId64,
					nextOffset);
			}
		}

		if (retval == E_ok) {
			kp = kpn->kpn_kp;

			if ((op == GET) || (op == DEL)) {
				KeyRecords keyRec(kp, kpSize);

				if (op == GET) {
					ki->ki_kidx = keyRec.get(ki);
				} else /* if (op == DEL) */ {
					ki->ki_kidx = keyRec.remove(ki);
				}

				if (ki->ki_kidx >= 0) {
					ki->ki_kpn = kpn;
					if (op == GET) {
						Log(DBG, caller,
							"key found: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						ki->ki_voff = kp->kp_keys[ki->ki_kidx].kr_voff;
						return E_ok;
					} else if (kp->kp_vcount > 0) {
						Log(DBG, caller,
							"key deleted: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						return keyFile->write(nextOffset, kp, kpSize);
					} else {
						Log(DBG, caller,
							"key found: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						Log(DBG, caller,
							"release page at offset %" PRId64,
							nextOffset);
						return E_ok;
					}
				}
			} else if (op == SET) {
				if (kp->kp_vcount < NUM_OF_KEYS_IN_PAGE(kpSize)) {
					KeyRecords keyRec(kp, kpSize); 
					ki->ki_kidx = keyRec.put(ki);
					if (ki->ki_kidx >= 0) {
						Log(DBG, caller,
							"key inserted: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						ki->ki_kpn = kpn;
						ki->ki_voff = kp->kp_keys[ki->ki_kidx].kr_voff;
						return keyFile->write(nextOffset, kp, kpSize);
					}
				}
			}

			ki->ki_lkp = kp;
			ki->ki_lkpoff = nextOffset;
			nextOffset = kp->kp_noff;
		}

		kpn = kpn->kpn_next;
	}

	while ((retval == E_ok) && (nextOffset != -1L)) {
		retval = cache->get(kpn, nextOffset);
		if (retval == E_ok) {
			hashTable->addKeyPageNode(ki->ki_hash, kpn);
		} else {
			Log(ERR, caller,
				"failed to read key page at offset %" PRId64,
				nextOffset);
		}

		if (retval == E_ok) {
			kp = kpn->kpn_kp;

			if ((op == GET) || (op == DEL)) {
				KeyRecords keyRec(kp, kpSize);

				if (op == GET) {
					ki->ki_kidx = keyRec.get(ki);
				} else /* if (op == DEL) */ {
					ki->ki_kidx = keyRec.remove(ki);
				}

				if (ki->ki_kidx >= 0) {
					ki->ki_kpn = kpn;
					if (op == GET) {
						Log(DBG, caller,
							"key found: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						ki->ki_voff = kp->kp_keys[ki->ki_kidx].kr_voff;
						return E_ok;
					} else if (kp->kp_vcount > 0) {
						Log(DBG, caller,
							"key deleted: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						return keyFile->write(nextOffset, kp, kpSize);
					} else {
						Log(DBG, caller,
							"key found: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						Log(DBG, caller,
							"release page at offset %" PRId64,
							nextOffset);
						return E_ok;
					}
				}
			} else if (op == SET) {
				if (kp->kp_vcount < NUM_OF_KEYS_IN_PAGE(kpSize)) {
					KeyRecords keyRec(kp, kpSize); 
					ki->ki_kidx = keyRec.put(ki);
					if (ki->ki_kidx >= 0) {
						Log(DBG, caller,
							"key inserted: page offset = %" PRId64
							", key index = %d",
							nextOffset, ki->ki_kidx);
						ki->ki_kpn = kpn;
						ki->ki_voff = kp->kp_keys[ki->ki_kidx].kr_voff;
						return keyFile->write(nextOffset, kp, kpSize);
					}
				}
			}

			ki->ki_lkp = kp;
			ki->ki_lkpoff = nextOffset;
			nextOffset = kp->kp_noff;
		}
	}

	if (retval == E_ok)
		retval = E_not_found;

	return retval;
}

/*
 * Adds a new key page to the system.
 *
 * @param [inout] ki - key information.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::addNewPage(key_info_t *ki)
{
	const char      *caller = "Rdb::addNewPage";
	int             retval = E_ok;
	int64_t         pageOffset = -1L;
	key_page_node_t *kpn;
	UnwindStack     ustk;

	retval = cache->get(kpn, -1L);
	if (retval != E_ok) {
		Log(ERR, caller, "failed to get a free key page");
		return retval;
	}

	key_page_t *kp = kpn->kpn_kp;

	InitKeyPage(kp, kpSize);
	kp->kp_hash = ki->ki_hash;
	kp->kp_poff = ki->ki_lkpoff;

	KeyRecords keyRec(kp, kpSize);
	ki->ki_kidx = keyRec.put(ki);
	Assert((ki->ki_kidx >= 0), __FILE__, __LINE__,
		"incorrect key index");

	retval = keyFile->write(&pageOffset, kp, kpSize);
	if (retval != E_ok) {
		Log(ERR, caller, "failed to write key to %s",
			keyFile->name());
	} else {
		Log(DBG, caller,
			"new page inserted: page offset = %" PRId64 ", key index = %d",
			pageOffset, ki->ki_kidx);

		kpn->kpn_kpoff = pageOffset;

		ustk.freePage(keyFile, pageOffset);
		ustk.writeFlags(keyFile, kp, pageOffset, KPAGE_DELETED);
	}

	if ((retval == E_ok) && ki->ki_lkp && (ki->ki_lkpoff != -1L)) {
		int64_t lastPageNextOffset = ki->ki_lkp->kp_noff;
		retval = keyFile->writeNextOffset(ki->ki_lkpoff, ki->ki_lkp, pageOffset);
		if (retval == E_ok) {
			ustk.writeNextOffset(keyFile, ki->ki_lkp, ki->ki_lkpoff, lastPageNextOffset);
		}
	}

	if (retval == E_ok) {
		ki->ki_kpn = kpn;
		hashTable->addKeyPageNode(ki->ki_hash, kpn);
	} else {
		cache->free(kpn);
	}

	ustk.unwind(retval);

	return retval;
}

/*
 * Backups up a database file.
 */
int
Rdb::backupFile(const char *oldName)
{
	char    newName[MAXPATHLEN + 1];

	strncpy(newName, oldName, MAXPATHLEN);
	strncat(newName, ".bkup", MAXPATHLEN);

	return snf::fs::rename(newName, oldName);
}

/*
 * Restores/recovers from the backed up file.
 */
int
Rdb::restoreFile(const char *oldName)
{
	char    newName[MAXPATHLEN + 1];

	strncpy(newName, oldName, MAXPATHLEN);
	strncat(newName, ".bkup", MAXPATHLEN);

	return snf::fs::rename(oldName, newName);
}

/*
 * Removes the backed-up file.
 */
int
Rdb::removeBackupFile(const char *oldName)
{
	char    newName[MAXPATHLEN + 1];

	strncpy(newName, oldName, MAXPATHLEN);
	strncat(newName, ".bkup", MAXPATHLEN);

	return snf::fs::remove_file(newName);
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
int
Rdb::setKeyPageSize(int kpsize)
{
	const char  *caller = "Rdb::setKetPageSize";

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

	std::lock_guard<std::mutex> guard(openMutex);
	if (!opened) {
		this->kpSize = kpsize;
		return E_ok;
	} else {
		Log(ERR, caller, "DB is open; cannot set key page size");
		return E_invalid_state;
	}
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
int
Rdb::setHashTableSize(int htsize)
{
	const char  *caller = "Rdb::setHashTableSize";

	if (htsize <= 0) {
		Log(ERR, caller,
			"invalid hash table size (%d)", htSize);
		return E_invalid_arg;
	}

	std::lock_guard<std::mutex> guard(openMutex);
	if (!opened) {
		this->htSize = NextPrime(htSize);
		return E_ok;
	} else {
		Log(ERR, caller, "DB is open; cannot set hash table size");
		return E_invalid_state;
	}
}

/**
 * Opens the database. The key page size and hash table size
 * must be set before opening the database for the first time.
 * Once the database is created, the key page and the hash table
 * size do not change. The only way to change them is to rebuild
 * the database.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::open()
{
	int     retval = E_ok;
	char    idxPath[MAXPATHLEN + 1];
	char    dbPath[MAXPATHLEN + 1];
	char    attrPath[MAXPATHLEN + 1];
	char    fdpPath[MAXPATHLEN + 1];

	std::lock_guard<std::mutex> guard(openMutex);
	if (opened) {
		return E_ok;
	}

	snprintf(idxPath, MAXPATHLEN, "%s%c%s", path.c_str(), snf::pathsep(), name.c_str());
	strncpy(dbPath, idxPath, MAXPATHLEN);
	strncpy(attrPath, idxPath, MAXPATHLEN);
	strncpy(fdpPath, idxPath, MAXPATHLEN);

	strncat(idxPath, ".idx", MAXPATHLEN);
	strncat(dbPath, ".db", MAXPATHLEN);
	strncat(attrPath, ".attr", MAXPATHLEN);
	strncat(fdpPath, ".fdp", MAXPATHLEN);

	std::unique_ptr<AttrFile> attrFile(DBG_NEW AttrFile(attrPath, 0022));
	retval = attrFile->open();
	if (retval != E_ok) {
		return retval;
	}

	retval = attrFile->read();
	if (retval != E_ok) {
		if (retval == E_eof_detected) {
			attrFile->setKeyPageSize(kpSize);
			attrFile->setHashTableSize(htSize);
			retval = attrFile->write();
		}
	}

	attrFile->close();

	if (retval != E_ok) {
		return retval;
	}

	kpSize = attrFile->getKeyPageSize();
	htSize = attrFile->getHashTableSize();

	std::unique_ptr<KeyFile> pKeyFile(DBG_NEW KeyFile(idxPath, 0022));
	retval = pKeyFile->open(options.syncIndexFile());
	if (retval != E_ok) {
		return retval;
	}

	std::unique_ptr<ValueFile> pValueFile(DBG_NEW ValueFile(dbPath, 0022));
	retval = pValueFile->open(options.syncDataFile());
	if (retval != E_ok) {
		return retval;
	}

	hashTable = DBG_NEW HashTable();
	retval = hashTable->allocate(htSize);
	if (retval != E_ok) {
		return retval;
	}

	keyFile = pKeyFile.release();
	valueFile = pValueFile.release();

	cache = DBG_NEW LRUCache(keyFile, kpSize, options.getMemoryUsage());

	retval = populateHashTable();
	if (retval == E_ok) {
		retval = populateFreePages(fdpPath);
	}

	if (retval != E_ok) {
		delete valueFile;
		delete keyFile;
		delete hashTable;
	} else {
		opened = true;
	}

	return retval;
}

/**
 * Gets the value for the key from the database.
 *
 * @param [in]    key    - database key.
 * @param [in]    klen   - database key length.
 * @param [out]   value  - value for the corresponding key.
 * @param [inout] vlen   - maximum value size on input,
 *                         actual value size on output.
 *
 * @return E_ok on success, E_not_found if the value is not
 * found, -ve error code on failure.
 */
int
Rdb::get(
	const char *key,
	int klen,
	char *value,
	int *vlen)
{
	const char      *caller = "Rdb::get";
	int             retval;
	int             hindex = -1;
	value_page_t    vp;
	key_info_t      ki;

	if ((key == 0) || (*key == '\0')) {
		Log(ERR, caller, "invalid key specified");
		return E_invalid_arg;
	}

	if (klen <= 0) {
		Log(ERR, caller, "invalid key length specified");
		return E_invalid_arg;
	}

	if (value == 0) {
		Log(ERR, caller, "invalid value specified");
		return E_invalid_arg;
	}

	if ((vlen == 0) || (*vlen <= 0)) {
		Log(ERR, caller, "invalid value length specified");
		return E_invalid_arg;
	}

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount++;
	}

	hindex = hash(key, klen, htSize);
	Assert(((hindex >= 0) && (hindex < htSize)), __FILE__, __LINE__,
		"invalid hash value (%d)", hindex);

	HTLockGuard guard(hashTable, hindex, false);

	SetKeyInfo(&ki, key, klen, hindex);

	retval = processKeyPages(&ki, GET);
	if (retval == E_ok) {
		ValidateKeyInfo(&ki, __FILE__, __LINE__);

		retval = valueFile->read(ki.ki_voff, &vp);
		if (retval != E_ok) {
			Log(ERR, caller, "failed to read value page at offset %" PRId64 " from %s",
				ki.ki_voff, valueFile->name());
		} else {
			Assert(!IsValuePageDeleted(&vp), __FILE__, __LINE__,
				"value is already deleted");
			Assert((klen == vp.vp_klen), __FILE__, __LINE__,
				"key length mismatch (expected %d, found %d)", klen, vp.vp_klen);
			Assert((memcmp(key, vp.vp_key, klen) == 0), __FILE__, __LINE__,
				"key mismatch");

			if (vp.vp_vlen > *vlen) {
				retval = E_insufficient_buffer;
			} else {
				if (vp.vp_vlen < *vlen) {
					*vlen = vp.vp_vlen;
				}
				memcpy(value, vp.vp_value, *vlen);
			}
		}
	}

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount--;
	}

	return retval;
}

/**
 * Set the key/value pair in the database. It is also used to
 * update the value in the database.
 *
 * @param [in]  key     - database key.
 * @param [in]  klen    - database key length.
 * @param [in]  value   - value for the corresponding key.
 * @param [in]  vlen    - value length.
 * @param [in]  updater - the updater object.
 *
 * - updater == 0
 *   If the key does not exist, a new key/value pair is
 *   added to the database.
 *   If the key already exists, its value is set to the new
 *   value specified.
 * - updater != 0
 *   If the key does not exist, a new key/value pair is
 *   added to the database.
 *   If the key already exists, its existing value is passed
 *   to the update() function of updater. Then the value
 *   returned by getUpdatedValue() is persisted in the
 *   database.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::set(
	const char *key,
	int klen,
	const char *value,
	int vlen,
	Updater *updater)
{
	const char      *caller = "Rdb::set";
	int             retval;
	int             hindex = -1;
	value_page_t    vp;
	key_info_t      ki;
	UnwindStack     ustk;

	if ((key == 0) || (*key == '\0')) {
		Log(ERR, caller, "invalid key specified");
		return E_invalid_arg;
	}

	if (klen <= 0) {
		Log(ERR, caller, "invalid key length specified");
		return E_invalid_arg;
	}

	if ((value == 0) || (*value == '\0')) {
		Log(ERR, caller, "invalid value specified");
		return E_invalid_arg;
	}

	if (vlen <= 0) {
		Log(ERR, caller, "invalid value length specified");
		return E_invalid_arg;
	}

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount++;
	}

	hindex = hash(key, klen, htSize);
	Assert(((hindex >= 0) && (hindex < htSize)), __FILE__, __LINE__,
		"invalid hash value (%d)", hindex);

	HTLockGuard guard(hashTable, hindex, true);

	SetKeyInfo(&ki, key, klen, hindex);

	InitValuePage(&vp, key, klen, value, vlen);

	retval = processKeyPages(&ki, GET);
	if (retval == E_ok) {
		ValidateKeyInfo(&ki, __FILE__, __LINE__);

		Log(DBG, caller, "key exists");

		if (updater) {
			Log(DBG, caller, "updating the value");

			retval = valueFile->read(ki.ki_voff, &vp);
			if (retval != E_ok) {
				Log(ERR, caller, "failed to read value page at offset %" PRId64 " from %s",
					ki.ki_voff, valueFile->name());
			} else {
				Assert(!IsValuePageDeleted(&vp), __FILE__, __LINE__,
					"value is already deleted");
				Assert((klen == vp.vp_klen), __FILE__, __LINE__,
					"key length mismatch (expected %d, found %d)", klen, vp.vp_klen);
				Assert((memcmp(key, vp.vp_key, klen) == 0), __FILE__, __LINE__,
					"key mismatch");

				retval = updater->update(vp.vp_value, vp.vp_vlen);
				if (retval == E_ok) {
					retval = updater->getUpdatedValue(vp.vp_value, &(vp.vp_vlen));
				}
			}
		}

		if (retval == E_ok) {
			retval = valueFile->write(ki.ki_voff, &vp);
			if (retval != E_ok) {
				Log(ERR, caller, "failed to write value to %s",
					valueFile->name());
			}
		}
	} else if (retval == E_not_found) {

		Log(DBG, caller, "writing a new value");

		retval = valueFile->write(&(ki.ki_voff), &vp);
		if (retval != E_ok) {
			Log(ERR, caller, "failed to write value to %s",
				valueFile->name());
		} else {
			ustk.freePage(valueFile, ki.ki_voff);
			ustk.writeFlags(valueFile, &vp, ki.ki_voff, VPAGE_DELETED);

			retval = processKeyPages(&ki, SET);
			if (retval == E_not_found) {
				retval = addNewPage(&ki);
			}
		}
	}

	ustk.unwind(retval);

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount--;
	}

	return retval;
}

/**
 * Removes the key/value pair from the database.
 *
 * @param [in]  key     - database key.
 * @param [in]  klen    - database key length.
 * 
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::remove(
	const char *key,
	int klen)
{
	const char  *caller = "Rdb::remove";
	int         retval;
	int         hindex = -1;
	key_info_t  ki;
	key_info_t  dki;
	UnwindStack ustk;

	if ((key == 0) || (*key == '\0')) {
		Log(ERR, caller, "invalid key specified");
		return E_invalid_arg;
	}

	if (klen <= 0) {
		Log(ERR, caller, "invalid key length specified");
		return E_invalid_arg;
	}

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount++;
	}

	hindex = hash(key, klen, htSize);
	Assert(((hindex >= 0) && (hindex < htSize)), __FILE__, __LINE__,
		"invalid hash value (%d)", hindex);

	HTLockGuard guard(hashTable, hindex, true);

	SetKeyInfo(&ki, key, klen, hindex);

	retval = processKeyPages(&ki, GET);
	if (retval == E_ok) {
		ValidateKeyInfo(&ki, __FILE__, __LINE__);

		// Mark the value page as deleted
		retval = valueFile->writeFlags(ki.ki_voff, 0, VPAGE_DELETED);
		if (retval != E_ok) {
			Log(ERR, caller, "failed to mark value page as deleted");
		} else {
			ustk.writeFlags(valueFile, 0, ki.ki_voff, 0);

			SetKeyInfo(&dki, key, klen, hindex);

			retval = processKeyPages(&dki, DEL);
			Assert((retval == E_ok), __FILE__, __LINE__,
				"unable to delete the key that was recently located");

			Assert((ki.ki_kpn == dki.ki_kpn), __FILE__, __LINE__,
				"found and deleted key page node mismatch");
			Assert((ki.ki_kpn->kpn_kp == dki.ki_kpn->kpn_kp), __FILE__, __LINE__,
				"found and deleted key page mismatch");
			Assert((ki.ki_kpn->kpn_kpoff == dki.ki_kpn->kpn_kpoff), __FILE__, __LINE__,
				"found and deleted key page offset mismatch");
			Assert((ki.ki_kidx == dki.ki_kidx), __FILE__, __LINE__,
				"found and deleted key index mismatch");

			// The key is deleted now

			if (ki.ki_kpn->kpn_kp->kp_vcount <= 0) {
				// It was the last key in the page

				key_page_t *prev_kp = 0, *next_kp = 0;
				key_page_node_t *prev_kpn, *next_kpn;
				int64_t prev_kpoff, next_kpoff;

				prev_kpoff = dki.ki_kpn->kpn_kp->kp_poff;
				prev_kpn = dki.ki_kpn->kpn_prev;
				next_kpoff = dki.ki_kpn->kpn_kp->kp_noff;
				next_kpn = dki.ki_kpn->kpn_next;

				Log(DBG, caller,
					"previous page offset = %" PRId64
					", next page offset = %" PRId64,
					prev_kpoff, next_kpoff);

				// The previous page is already loaded. If there
				// is a next page, load it as we need to update
				// its previous page offset.

				if ((next_kpoff != -1L) && (next_kpn == 0)) {
					retval = cache->get(next_kpn, next_kpoff);
					if (retval == E_ok) {
						hashTable->addKeyPageNode(dki.ki_hash, next_kpn);
					} else {
						Log(ERR, caller,
							"failed to read key page at offset %" PRId64,
							next_kpoff);
					}
				}

				if (retval == E_ok) {
					if ((prev_kpoff != -1L) && (prev_kpn != 0)) {
						prev_kp = prev_kpn->kpn_kp;
					}

					if ((next_kpoff != -1L) && (next_kpn != 0)) {
						next_kp = next_kpn->kpn_kp;
					}

					// Mark the key page as deleted
					retval = keyFile->writeFlags(dki.ki_kpn->kpn_kpoff,
								dki.ki_kpn->kpn_kp, KPAGE_DELETED);
					if (retval != E_ok) {
						Log(ERR, caller,
							"failed to mark key page at offset %" PRId64 " as deleted",
							dki.ki_kpn->kpn_kpoff);
					} else {
						ustk.writeFlags(keyFile, dki.ki_kpn->kpn_kp,
							dki.ki_kpn->kpn_kpoff, 0);

						int64_t offset = -1L;

						// Update next offset of the previous page
						if (prev_kp) {
							offset = prev_kp->kp_noff;
							retval = keyFile->writeNextOffset(prev_kpoff, prev_kp, next_kpoff);
							if (retval != E_ok) {
								Log(ERR, caller, "failed to update key page at offset %" PRId64
									" to %s", prev_kpoff, keyFile->name());
							} else {
								ustk.writeNextOffset(keyFile, prev_kp, prev_kpoff, offset);
							}
						}

						// Update previous offset of the next page
						if ((retval == E_ok) && next_kp) {
							offset = next_kp->kp_poff;
							retval = keyFile->writePrevOffset(next_kpoff, next_kp, prev_kpoff);
							if (retval != E_ok) {
								Log(ERR, caller, "failed to update key page at offset %" PRId64
									" to %s", next_kpoff, keyFile->name());
							} else {
								ustk.writePrevOffset(keyFile, next_kp, next_kpoff, offset);
							}
						}

						if (retval == E_ok) {
							// Update other data structures
							offset = dki.ki_kpn->kpn_kpoff;
							hashTable->removeKeyPageNode(hindex, dki.ki_kpn);
							cache->free(dki.ki_kpn);
							retval = keyFile->freePage(offset);
						}
					}
				}
			}

			if (retval == E_ok) {
				retval = valueFile->freePage(ki.ki_voff);
			}
		}
	}

	ustk.unwind(retval);

	{
		std::lock_guard<std::mutex> guard(opMutex);
		opCount--;
	}

	return retval;
}

/**
 * Rebuilds the database. It does the following:
 * 1. Backs up the database.
 * 2. Create a new database.
 * 3. Reads the key/value from the backed up database files
 *    and adds them to the new database created.
 * 4. Cleans up the backed up files.
 *
 * Rebuilding database serves two purposes:
 * 1. Defragment the database.
 * 2. Provides for a way to change the key page and
 *    hash table size.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::rebuild()
{
	const char      *caller = "Rdb::rebuild";
	int             retval = E_ok;
	char            idxPath[MAXPATHLEN + 1];
	char            dbPath[MAXPATHLEN + 1];
	char            dbPathBkup[MAXPATHLEN + 1];
	char            attrPath[MAXPATHLEN + 1];
	char            fdpPath[MAXPATHLEN + 1];
	int64_t         offset = 0;
	value_page_t    vp;

	{
		std::lock_guard<std::mutex> guard1(openMutex);
		if (opened) {
			Log(ERR, caller, "DB is open; close it before rebuilding");
			return E_invalid_state;
		}

		snprintf(idxPath, MAXPATHLEN, "%s%c%s", path.c_str(), snf::pathsep(), name.c_str());
		strncpy(dbPath, idxPath, MAXPATHLEN);
		strncpy(attrPath, idxPath, MAXPATHLEN);
		strncpy(fdpPath, idxPath, MAXPATHLEN);

		strncat(idxPath, ".idx", MAXPATHLEN);
		strncat(dbPath, ".db", MAXPATHLEN);
		strncat(attrPath, ".attr", MAXPATHLEN);
		strncat(fdpPath, ".fdp", MAXPATHLEN);

		if ((retval = backupFile(idxPath)) != E_ok)
			return retval;

		if ((retval = backupFile(dbPath)) != E_ok) {
			restoreFile(idxPath);
			return retval;
		}

		if ((retval = backupFile(attrPath)) != E_ok) {
			restoreFile(dbPath);
			restoreFile(idxPath);
			return retval;
		}

		if ((retval = backupFile(fdpPath)) != E_ok) {
			restoreFile(attrPath);
			restoreFile(dbPath);
			restoreFile(idxPath);
			return retval;
		}
	}

	if ((retval = open()) != E_ok) {
		restoreFile(fdpPath);
		restoreFile(attrPath);
		restoreFile(dbPath);
		restoreFile(idxPath);
		return retval;
	}

	strncpy(dbPathBkup, dbPath, MAXPATHLEN);
	strncat(dbPathBkup, ".bkup", MAXPATHLEN);
	ValueFile vf(dbPathBkup, 0022);
	retval = vf.open(false);
	if (retval != E_ok) {
		restoreFile(fdpPath);
		restoreFile(attrPath);
		restoreFile(dbPath);
		restoreFile(idxPath);
		return retval;
	}

	while ((retval = vf.read(offset, &vp)) == E_ok) {
		if ((vp.vp_flags & VPAGE_DELETED) != VPAGE_DELETED) {
			retval = set(vp.vp_key, vp.vp_klen, vp.vp_value, vp.vp_vlen);
			if (retval != E_ok) 
				break;
		}
		offset += (int64_t) sizeof(vp);
	}

	if (retval == E_eof_detected) {
		retval = E_ok;
	}

	vf.close();

	close();

	if (retval != E_ok) {
		restoreFile(fdpPath);
		restoreFile(attrPath);
		restoreFile(dbPath);
		restoreFile(idxPath);
	} else {
		removeBackupFile(fdpPath);
		removeBackupFile(attrPath);
		removeBackupFile(dbPath);
		removeBackupFile(idxPath);
	}

	return retval;
}

/**
 * Closes the database.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Rdb::close()
{
	std::lock_guard<std::mutex> guard1(openMutex);
	if (!opened) {
		return E_ok;
	}

	std::lock_guard<std::mutex> guard2(opMutex);
	if (opCount > 0) {
		return E_try_again;
	}

	if (valueFile) {
		delete valueFile;
		valueFile = 0;
	}

	if (keyFile) {
		delete keyFile;
		keyFile = 0;
	}

	if (cache) {
		delete cache;
		cache = 0;
	}

	if (hashTable) {
		delete hashTable;
		hashTable = 0;
	}

	opened = false;

	return E_ok;
}
