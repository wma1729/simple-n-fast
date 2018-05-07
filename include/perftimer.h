#ifndef _SNF_PERFTIMER_H_
#define _SNF_PERFTIMER_H_

#include "common.h"
#include <chrono>

/**
 * Used to time code (in micro-seconds).
 */
class PerformanceTimer
{
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_tp;

public:
	PerformanceTimer();
	PerformanceTimer(const PerformanceTimer &);
	~PerformanceTimer()
	{
	}

	PerformanceTimer & operator=(const PerformanceTimer &);
	void now();
	int64_t operator-(const PerformanceTimer &);
};

#endif // _SNF_PERFTIMER_H_
