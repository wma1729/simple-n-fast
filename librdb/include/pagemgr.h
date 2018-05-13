#ifndef _PAGEMGR_H_
#define _PAGEMGR_H_

#include <stack>
#include <mutex>
#include "common.h"

class PageMgr
{
private:
	char                *pool;
	size_t              poolSize;
	int                 numOfPages;
	int                 numOfFreePages;
	int                 pageSize;
	std::stack<char *>  nextFreePage;
	std::mutex          mutex;


public:
	PageMgr(int pageSize, int memUsage);

	~PageMgr()
	{
		if (pool) {
			::free(pool);
			pool = 0;
		}

		std::lock_guard<std::mutex> guard(mutex);

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
