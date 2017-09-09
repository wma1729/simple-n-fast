#ifndef _SNF_PERFTIMER_H_
#define _SNF_PERFTIMER_H_

#include "common.h"

/**
 * Used to time code (in micro-seconds).
 */
class PerformanceTimer
{
private:
#if defined(WINDOWS)
	static	LARGE_INTEGER   freq;
	        LARGE_INTEGER   counter;
#else
	struct timeval  tv;
#endif

public:
	PerformanceTimer();
	PerformanceTimer(const PerformanceTimer &);
	~PerformanceTimer()
	{
	}

	PerformanceTimer & operator=(const PerformanceTimer &);
	void record();
	int64_t operator-(const PerformanceTimer &);
};

#endif // _SNF_PERFTIMER_H_
