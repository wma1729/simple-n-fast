#ifndef _FDPMGR_H_
#define _FDPMGR_H_

#include <stack>
#include <mutex>
#include "file.h"

/**
 * Manage a stack of free disk page offsets
 * so that free disk pages can be reused.
 * Use get() to get a free disk page and
 * free() to mark the disk page as free.
 *
 * Internally, the class manages a stack of
 * free disk pages. get() pops a free disk
 * page offset from the stack and free()
 * pushes the free disk page offset on the
 * stack.
 *
 * In the beginning there are no free disk page
 * offsets and the stack is empty. In this
 * scenario, the caller is expected to set the
 * first free disk page offset(s) by calling
 * free(<file_size>) one or more times.
 */
class FreeDiskPageMgr
{
private:
	int                 pageSize;
	std::stack<int64_t> nextFreeOffset;
	File                *file;
	int64_t             fsize;
	std::mutex          mutex;

	int addOffsetToFile(int64_t);
	int removeOffsetFromFile();
	
public:
	/**
	 * Constructs the free disk page manager object.
	 *
	 * @param [in] pageSize - The disk page size. It is
	 *                        different for key and value file.
	 * @param [in] fmgr     - The file manager. Used to persist
	 *                        the free disk page offsets. It
	 *                        is used only for value file as the
	 *                        index db is fully read when the
	 *                        database opens and the stack is
	 *                        created from scratch.
	 */
	FreeDiskPageMgr(int pageSize, File *file = 0)
	{
		this->pageSize = pageSize;
		this->file = file;
		this->fsize = 0;
	}

	/**
	 * Destroys the free disk page manager object.
	 */
	~FreeDiskPageMgr()
	{
		if (file) {
			delete file;
			file = 0;
		}

		std::lock_guard<std::mutex> guard(mutex);
		while (!nextFreeOffset.empty())
			nextFreeOffset.pop();
	}

	int init();
	int64_t get();
	int free(int64_t);
	int reset();

	/**
	 * Returns the number of free disk pages
	 * available (now).
	 */
	size_t size()
	{
		std::lock_guard<std::mutex> guard(mutex);
		return nextFreeOffset.size();
	}
};

#endif // _FDPMGR_H_
