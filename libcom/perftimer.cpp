#include "perftimer.h"
#include "log.h"

#if defined(_WIN32)
LARGE_INTEGER PerformanceTimer::freq = { 0 };
#endif

/**
 * Constructs the performance timer object.
 */
PerformanceTimer::PerformanceTimer()
{
#if defined(_WIN32)
	counter.QuadPart = 0L;
#else
	tv.tv_sec = 0;
	tv.tv_usec = 0;
#endif
}

/**
 * Constructs the performance timer object from another
 * performance timer object.
 *
 * @param [in] timer Peformance timer object.
 */
PerformanceTimer::PerformanceTimer(const PerformanceTimer &timer)
{
#if defined(_WIN32)
	this->counter = timer.counter;
#else
	this->tv.tv_sec = timer.tv.tv_sec;
	this->tv.tv_usec = timer.tv.tv_usec;
#endif
}

/**
 * Records the time now in the timer.
 */
void
PerformanceTimer::record()
{
#if defined(_WIN32)
	BOOL retval = FALSE;

	if (freq.QuadPart == 0L) {
		retval = QueryPerformanceFrequency(&freq);
		Assert((retval != FALSE), __FILE__, __LINE__, GetLastError(),
			"failed to query the performance counter frequency");
	}

	retval = QueryPerformanceCounter(&counter);
	Assert((retval != FALSE), __FILE__, __LINE__, GetLastError(),
		"failed to query the performance counter");
#else
	int retval = gettimeofday(&tv, 0);
	Assert((retval == 0), __FILE__, __LINE__, errno,
		"failed to get time of day");
#endif
}

/**
 * Assigns the specified timer value to the current timer.
 *
 * @param [in] timer Peformance timer object.
 */
PerformanceTimer &
PerformanceTimer::operator=(const PerformanceTimer &timer)
{
	if (this != &timer) {
#if defined(_WIN32)
		this->counter = timer.counter;
#else
		this->tv.tv_sec = timer.tv.tv_sec;
		this->tv.tv_usec = timer.tv.tv_usec;
#endif
	}

	return *this;
}

/**
 * Finds the time elapsed between the current and specified
 * timer in microseconds.
 *
 * @param [in] timer Peformance timer object.
 *
 * @return elpased time in microseconds.
 */
int64_t
PerformanceTimer::operator-(const PerformanceTimer &timer)
{
	if (this == &timer) {
		return 0L;
	}

	int64_t elapsed;

#if defined(_WIN32)
	elapsed = int64_t((((this->counter.QuadPart - timer.counter.QuadPart) * 1000000) /
						this->freq.QuadPart));
#else
	elapsed = int64_t((this->tv.tv_sec - timer.tv.tv_sec) * 1000000);
	elapsed += int64_t(this->tv.tv_usec - timer.tv.tv_usec);
#endif

	return elapsed;
}
