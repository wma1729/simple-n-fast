#include <cstddef>
#include "dbfiles.h"
#include "logmgr.h"
#include "error.h"

/*
 * Opens the database file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
static int
OpenFile(snf::file *file, bool sync = false)
{
	int                  retval = E_ok;
	int                  oserr = 0;
	snf::file::open_flags oflags;

	oflags.o_read = true;
	oflags.o_write = true;
	oflags.o_create = true;
	if (sync)
		oflags.o_sync = true;

	retval = file->open(oflags, 0600, &oserr);
	if (retval != E_ok) {
		ERROR_STRM(nullptr, oserr)
			<< "failed to open file " << file->name()
			<< snf::log::record::endl;
	}

	return retval;
}

/**
 * Reads the database file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
static int
ReadFile(snf::file *file, int64_t offset, void *buf, int toRead)
{
	int     retval = E_ok;
	int     oserr = 0;
	int     bRead = 0;

	retval = file->read(offset, buf, toRead, &bRead, &oserr);
	if (retval != E_ok) {
		ERROR_STRM(nullptr, oserr)
			<< "failed to read file " << file->name()
			<< " at offset " << offset
			<< snf::log::record::endl;
	} else if (bRead == 0) {
		DEBUG_STRM(nullptr)
			<< "end of file detected at offset " << offset
			<< snf::log::record::endl;
		retval = E_eof_detected;
	} else if (bRead != toRead) {
		ERROR_STRM(nullptr)
			<< "expected to read " << toRead
			<< " bytes, read only " << bRead << " bytes"
			<< snf::log::record::endl;
		retval = E_read_failed;
	}

	return retval;
}

/**
 * Writes the database file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
static int
WriteFile(snf::file *file, int64_t offset, const void *buf, int toWrite)
{
	int     retval = E_ok;
	int     oserr = 0;
	int     bWritten = 0;

	retval = file->write(offset, buf, toWrite, &bWritten, &oserr);
	if (retval != E_ok) {
		ERROR_STRM(nullptr, oserr)
			<< "failed to write file " << file->name()
			<< " at offset " << offset
			<< snf::log::record::endl;
	} else if (bWritten != toWrite) {
		ERROR_STRM(nullptr)
			<< "expected to write " << toWrite
			<< " bytes, wrote only " << bWritten << " bytes"
			<< snf::log::record::endl;
		retval = E_write_failed;
	}

	return retval;
}

/**
 * Opens the database attributes file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
AttrFile::open()
{
	return OpenFile(this);
}

/**
 * Reads database attributes from the file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
AttrFile::read()
{
	return ReadFile(this, 0L, &dbAttr, int(sizeof(dbAttr)));
}

/**
 * Writes database attributes to the file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
AttrFile::write()
{
	return WriteFile(this, 0L, &dbAttr, int(sizeof(dbAttr)));
}

/**
 * Opens the database key file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::open(bool sync)
{
	return OpenFile(this, sync);
}

/**
 * Reads key page at the given offset from the key file.
 *
 * @param [in]  offset - key file offset.
 * @param [out] buf    - key page buffer.
 * @param [in]  toRead - bytes to read.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::read(int64_t offset, void *buf, int toRead)
{
	std::lock_guard<std::mutex> guard(mutex);
	return ReadFile(this, offset, buf, toRead);
}

/**
 * Writes key page at the given offset in the key file.
 *
 * @param [in] offset  - key file offset.
 * @param [in] buf     - buffer to write.
 * @param [in] toWrite - bytes to write.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::write(int64_t offset, const void *buf, int toWrite)
{
	std::lock_guard<std::mutex> guard(mutex);
	return WriteFile(this, offset, buf, toWrite);
}

/**
 * Obtains the key offset and write the key page
 * at the offset in the key file.
 *
 * @param [out] offset  - key file offset where the page is written.
 * @param [in]  buf     - key page buffer.
 * @param [in]  toWrite - bytes to write.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::write(int64_t *offset, const void *buf, int toWrite)
{
	int         retval = E_ok;
	int64_t     newOffset = fdpMgr->get();

	if (newOffset < 0) {
		ERROR_STRM("KeyFile")
			<< "failed to get next free disk block"
			<< snf::log::record::endl;
		return int(newOffset);
	}

	retval = write(newOffset, buf, toWrite);
	if (retval != E_ok) {
		fdpMgr->free(newOffset);
	} else {
		*offset = newOffset;
	}

	return retval;
}

/**
 * Writes the flags field in the key page.
 *
 * @param [in]  offset  - page offset in the key file.
 * @param [out] kp      - key page. If kp is not NULL,
 *                        the flags in the key page
 *                        record is updated as well.
 * @param [in]  flags   - new flags to apply.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::writeFlags(int64_t offset, key_page_t *kp, int flags)
{
	int retval = E_ok;

	offset += offsetof(key_page_t, kp_flags);
	retval = write(offset, &flags, int(sizeof(flags)));
	if (retval == E_ok) {
		if (kp) {
			int oflags = kp->kp_flags;
			kp->kp_flags = flags;

			LOG_DEBUG("KeyFile",
				"flags for page at offset %" PRId64 " changed: old 0x%04x, new 0x%04x",
				offset, oflags, flags);
		}
	} else {
		LOG_ERROR("KeyFile",
			"failed to set flags to 0x%04x for page at offset %" PRId64,
			flags, offset);
	}

	return retval;
}

/**
 * Writes the previous offset field in the key page.
 *
 * @param [in]  offset      - page offset in the key file.
 * @param [out] kp          - key page. If kp is not NULL,
 *                            the previous offset in the key
 *                            page record is updated as well.
 * @param [in]  prevOffset  - new previous offset.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::writePrevOffset(int64_t offset, key_page_t *kp, int64_t prevOffset)
{
	int retval = E_ok;

	offset += offsetof(key_page_t, kp_poff);
	retval = write(offset, &prevOffset, int(sizeof(prevOffset)));
	if (retval == E_ok) {
		if (kp) {
			int64_t opoff = kp->kp_poff;
			kp->kp_poff = prevOffset;

			LOG_DEBUG("KeyFile",
				"previous offset for page at offset %" PRId64
				" changed: old %" PRId64 ", new %" PRId64,
				offset, opoff, prevOffset);
		}
	} else {
		LOG_ERROR("KeyFile",
			"failed to set previous offset to %" PRId64
			" for page at offset %" PRId64,
			prevOffset, offset);
	}

	return retval;
}

/**
 * Writes the next offset field in the key page.
 *
 * @param [in]  offset      - page offset in the key file.
 * @param [out] kp          - key page. If kp is not NULL,
 *                            the next offset in the key
 *                            page record is updated as well.
 * @param [in]  nextOffset  - new next offset.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::writeNextOffset(int64_t offset, key_page_t *kp, int64_t nextOffset)
{
	int retval = E_ok;

	offset += offsetof(key_page_t, kp_noff);
	retval = write(offset, &nextOffset, int(sizeof(nextOffset)));
	if (retval == E_ok) {
		if (kp) {
			int64_t onoff = kp->kp_noff;
			kp->kp_noff = nextOffset;

			LOG_DEBUG("KeyFile",
				"next offset for page at offset %" PRId64
				" changed: old %" PRId64 ", new %" PRId64,
				offset, onoff, nextOffset);
		}
	} else {
		LOG_ERROR("KeyFile",
			"failed to set next offset to %" PRId64
			" for page at offset %" PRId64,
			nextOffset, offset);
	}

	return retval;
}

/**
 * Frees the key page at the specified offset.
 *
 * @param [in] offset - file offset.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
KeyFile::freePage(int64_t offset)
{
	if (fdpMgr) {
		return fdpMgr->free(offset);
	} else {
		ERROR_STRM("KeyFile")
			<< "free disk page manager is not set"
			<< snf::log::record::endl;
		return E_invalid_state;
	}
}

/**
 * Opens the database value file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::open(bool sync)
{
	return OpenFile(this, sync);
}

/**
 * Reads the value page at the given offset.
 *
 * @param [in]  offset - page offset in the value file.
 * @param [out] vp     - value page.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::read(int64_t offset, value_page_t *vp)
{
	std::lock_guard<std::mutex> guard(mutex);
	return ReadFile(this, offset, vp, int(sizeof(*vp)));
}

/**
 * Reads the flag from the value page which is located
 * at the specified offset.
 *
 * @param [in]  offset - page offset in the value file.
 * @param [out] flags  - value page flags.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::readFlags(int64_t offset, int *flags)
{
	std::lock_guard<std::mutex> guard(mutex);
	offset += offsetof(value_page_t, vp_flags);
	return ReadFile(this, offset, flags, int(sizeof(int)));
}

/**
 * Writes value page at the specified offset in the file.
 *
 * @param [in] offset - file offset where the value page
 *                      is to be written.
 * @param [in] vp     - value page.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::write(int64_t offset, const value_page_t *vp)
{
	std::lock_guard<std::mutex> guard(mutex);
	return WriteFile(this, offset, vp, int(sizeof(*vp)));
}

/**
 * Writes value page at the next free slot in the file.
 *
 * @param [out] offset - File offset where the value page
 *                       is written.
 * @param [in]  vp     - value page
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::write(int64_t *offset, const value_page_t *vp)
{
	int     retval = E_ok;
	int64_t newOffset = fdpMgr->get();

	if (newOffset < 0) {
		ERROR_STRM("ValueFile")
			<< "failed to get next free disk page"
			<< snf::log::record::endl;
		return int(newOffset);
	}

	retval = write(newOffset, vp);
	if (retval != E_ok) {
		fdpMgr->free(newOffset);
	} else {
		*offset = newOffset;
	}

	return E_ok;
}

/**
 * Write flags in the value page which is located
 * at the specified index.
 *
 * @param [in]  offset - page offset in the value file.
 * @param [out] vp     - value page. If vp is not NULL,
 *                       the flags field in the value
 *                       page record is updated as well.
 * @param [in]  flags  - new flag to apply.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::writeFlags(int64_t offset, value_page_t *vp, int flags)
{
	int retval = E_ok;

	{
		std::lock_guard<std::mutex> guard(mutex);
		offset += offsetof(value_page_t, vp_flags);
		retval = WriteFile(this, offset, &flags, int(sizeof(flags)));
	}

	if (retval == E_ok) {
		if (vp)
			vp->vp_flags = flags;
	} else {
		LOG_ERROR("ValueFile", "failed to set flags to 0x%04x", flags);
	}

	return retval;
}

/**
 * Frees the value page at the specified offset.
 *
 * @param [in] offset - file offset.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
ValueFile::freePage(int64_t offset)
{
	if (fdpMgr) {
		return fdpMgr->free(offset);
	} else {
		ERROR_STRM("ValueFile")
			<< "free disk page manager is not set"
			<< snf::log::record::endl;
		return E_invalid_state;
	}
}
