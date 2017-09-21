#if !defined(WINDOWS)
#include <sys/mman.h>
#endif

#include "util.h"
#include "rdb/pagemgr.h"

/**
 * Get the physical memory size in bytes.
 */
static size_t
GetMemorySize()
{
#if defined(WINDOWS)
	MEMORYSTATUSEX mstat;
	mstat.dwLength = sizeof(mstat);
	GlobalMemoryStatusEx(&mstat);
	return size_t(mstat.ullTotalPhys);

#else
	long pageSize = sysconf(_SC_PAGESIZE);
	long numOfPage = sysconf(_SC_PHYS_PAGES);
	int64_t totalMemory = int64_t(pageSize) * int64_t(numOfPage);
	return size_t(totalMemory);
#endif
}

PageMgr::PageMgr(size_t pageSize, int memUsage)
	: pageSize(pageSize),
	  mutex()
{
	pool = 0;
	poolSize = GetMemorySize() * (memUsage / 100);

	do {
		if (poolSize < 0) {
			break;
		}

		pool = (char *)malloc(poolSize);
		if (pool == 0) {
			// reduce by 100 MB on every failure
			poolSize -= (100 * 1024 * 1024);
		}
	} while (pool == 0);

	Assert((pool != 0), __FILE__, __LINE__, errno,
		"unable to allocate memory for page pool");

	numOfPages = poolSize / pageSize;
	numOfFreePages = numOfPages;

#if !defined(WINDOWS)
	posix_madvise(pool, poolSize, MADV_WILLNEED);
#endif

	nextFreePage.push(pool);
}

void *
PageMgr::get()
{
	char *addr = 0;

	MutexGuard guard(mutex);

	if (!nextFreePage.empty() && (numOfFreePages > 0)) {
		addr = nextFreePage.top();
		nextFreePage.pop();
		memset(addr, 0, pageSize);
		numOfFreePages--;

		if (nextFreePage.empty()) {
			char *nfaddr = addr + pageSize;
			if (nfaddr < (pool + poolSize)) {
				nextFreePage.push(nfaddr);
			}
		}
	}

	return addr;
}

void
PageMgr::free(void *addr)
{
	char *caddr = (char *)addr;
	ptrdiff_t diff;

	Assertion(((caddr >= pool) && (caddr < (pool + poolSize))), __FILE__, __LINE__,
		"out-of-bound memory address (0x%x), range [0x%x, 0x%x)",
		caddr, pool, pool + poolSize);

	diff = caddr - pool;
	Assertion(((diff % pageSize) == 0), __FILE__, __LINE__,
		"address (%p) is not correctly aligned",
		addr);

	MutexGuard guard(mutex);

	nextFreePage.push(caddr);
	numOfFreePages++;
}
