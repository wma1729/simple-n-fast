#include "perftimer.h"

using namespace std::chrono;

/**
 * Constructs the performance timer object.
 */
PerformanceTimer::PerformanceTimer()
{
	m_tp = high_resolution_clock::now();
}

/**
 * Constructs the performance timer object from another
 * performance timer object.
 *
 * @param [in] timer Peformance timer object.
 */
PerformanceTimer::PerformanceTimer(const PerformanceTimer &timer)
{
	m_tp = timer.m_tp;
}

/**
 * Records the time now in the timer.
 */
void
PerformanceTimer::now()
{
	m_tp = high_resolution_clock::now();
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
		m_tp = timer.m_tp;
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
	return duration_cast<microseconds>(m_tp - timer.m_tp).count();
}
