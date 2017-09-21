#ifndef _PAGEMGR_H_
#define _PAGEMGR_H_

#include <stack>
#include "util.h"

class PageMgr
{
private:
	char                *pool;
	size_t              poolSize;
	int                 numOfPages;
	int                 numOfFreePages;
	int                 pageSize;
	std::stack<char *>  nextFreePage;
	Mutex               mutex;


public:
	PageMgr(size_t pageSize, int memUsage);

	~PageMgr()
	{
		if (pool) {
			::free(pool);
			pool = 0;
		}

		MutexGuard guard(mutex);

		while (!nextFreePage.empty())
			nextFreePage.pop();

	}

	int getNumberOfPages() const
	{
		return numOfPages;
	}

	int getNumberOfFreePages() const
	{
		return numOfFreePages;
	}

	void *get();
	void free(void *);
};

#endif // _PAGEMGR_H_
