#include "rdb/fdpmgr.h"
#include "log.h"
#include "error.h"

/*
 * Adds the offset to the file.
 *
 * This function assumes that the file pointer is
 * already set correctly (i.e. at the end of the file).
 *
 * @param [in] offset - The file offset to persist.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FreeDiskPageMgr::addOffsetToFile(int64_t offset)
{
	int retval = E_ok;

	if (file) {
		const char  *caller = "FreeDiskPageMgr::addOffsetToFile";
		int         oserr = 0;
		int         toWrite = int(sizeof(offset));
		int         bWritten = 0;

		retval = file->write(&offset, toWrite, &bWritten, &oserr);
		if (retval != E_ok) {
			Log(ERR, caller, oserr,
				"failed to write free disk page offset (%" PRId64 ") to file %s",
				offset, file->getFileName());
		} else if (bWritten != toWrite) {
			Log(ERR, caller, "expected to write %d bytes, written only %d bytes",
				toWrite, bWritten);
			retval = E_write_failed;
		} else {
			fsize += sizeof(offset);
		}
	}

	return retval;
}

/*
 * Removes the last offset from the file.
 * This function simply truncate the filesize by
 * sizeof(int64_t).
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FreeDiskPageMgr::removeOffsetFromFile()
{
	int retval = 0;

	if (file) {
		const char  *caller = "FreeDiskPageMgr::removeOffsetFromFile";
		int         oserr = 0;

		fsize -= sizeof(int64_t);
		retval = file->truncate(fsize, &oserr);
		if (retval != E_ok) {
			fsize += sizeof(int64_t);
			Log(ERR, caller, "failed to remove last free disk page offset from file %s",
				file->getFileName());
		}
	}

	return retval;
}

/*
 * Reads the free disk pages from the file and populate
 * the stack. It has a side-effect of setting the file
 * pointer to the correct position (i.e. the end of the
 * file.
 *
 * Note: Only called once at start-up.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FreeDiskPageMgr::init()
{
	int retval = E_ok;

	if (file) {
		const char  *caller = "FreeDiskPageMgr::init";
		int         oserr = 0;
		int64_t     offset = 0L;
		int         toRead = int(sizeof(offset));
		int         bRead = 0;

		retval = file->seek(0L, &oserr);
		if (retval != E_ok) {
			Log(ERR, caller, oserr,
				"unable to seek to the start of the file %s",
				file->getFileName());
		} else {
			do {
				retval = file->read(&offset, toRead, &bRead, &oserr);
				if (retval != E_ok) {
					Log(ERR, caller, oserr,
						"failed to read offset from file %s at offset %" PRId64,
						file->getFileName(), fsize);
				} else if (bRead == 0) {
					retval = E_eof_detected;
					break;
				} else if (bRead != toRead) {
					Log(ERR, caller,
						"expected to read %d bytes, read only %d bytes",
						toRead, bRead);
					retval = E_write_failed;
				} else {
					fsize += bRead;
					nextFreeOffset.push(offset);
				}
			} while (retval == E_ok);

			if (retval != E_eof_detected) {
				retval = E_ok;
			}
		}
	}

	return retval;
}

/**
 * Gets the next free disk page. If the offset is
 * the last element on the stack (which also means
 * we are going to append to the file), a new
 * element with offset equal to [offset just fetched
 * plus the page size] is pushed on the stack.
 *
 * @return the next free disk page offset (+ve value) on
 * success, -ve error code on failure.
 */
int64_t
FreeDiskPageMgr::get()
{
	std::lock_guard<std::mutex> guard(mutex);

	Assert(!nextFreeOffset.empty(), __FILE__, __LINE__,
		"empty offset stack");

	int64_t next = nextFreeOffset.top();
	nextFreeOffset.pop();

	int retval = removeOffsetFromFile();
	if (retval != E_ok) {
		// undo pop 
		nextFreeOffset.push(next);
		return retval;
	}

	if (nextFreeOffset.empty()) {
		nextFreeOffset.push(next + pageSize); 

		retval = addOffsetToFile(next + pageSize);
		if (retval != E_ok) {
			// undo push
			nextFreeOffset.pop();
			// undo pop
			nextFreeOffset.push(next);
			return retval;
		}
	}

	return next;
}

/**
 * Frees the disk page offset. It is added to the stack
 * for re-use.
 *
 * @param [in] offset - The free disk page offset that is
 *                      available for re-use.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FreeDiskPageMgr::free(int64_t offset)
{
	Assert((offset >= 0), __FILE__, __LINE__,
		"invalid offset (%" PRId64 ") specified", offset);
	Assert(((offset % pageSize) == 0), __FILE__, __LINE__,
		"offset (%" PRId64 ") is not correctly aligned", offset);

	std::lock_guard<std::mutex> guard(mutex);

	nextFreeOffset.push(offset);

	int retval = addOffsetToFile(offset);
	if (retval != E_ok) {
		// undo push
		nextFreeOffset.pop();
	}

	return retval;
}

/**
 * Resets everything. Use with caution (preferably
 * at start-up only).
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
FreeDiskPageMgr::reset()
{
	if (file) {
		const char  *caller = "FreeDiskPageMgr::reset";
		int         oserr = 0;

		int retval = file->truncate(0L, &oserr);
		if (retval != E_ok) {
			Log(ERR, caller, oserr, "failed to truncate file %s to size 0",
				file->getFileName());
			return retval;
		}
		fsize = 0;
	}

	while (!nextFreeOffset.empty())
		nextFreeOffset.pop();

	return E_ok;
}
