#include "timeutil.h"
#include <ctime>

namespace snf {

local_time::local_time()
{
#if defined(_WIN32)

	SYSTEMTIME      st;
	FILETIME        ft;
	ULARGE_INTEGER  current;
	ULARGE_INTEGER  epoch;

	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &ft);
	current.LowPart = ft.dwLowDateTime;
	current.HighPart = ft.dwHighDateTime;
	epoch.QuadPart = 116444736000000000I64;

	m_year = st.wYear;
	m_month = st.wMonth;
	m_day = st.wDay;
	m_hour = st.wHour;
	m_minute = st.wMinute;
	m_second = st.wSecond;
	m_msec = st.wMilliseconds;
	m_epoch = static_cast<int64_t>((current.QuadPart - epoch.QuadPart) / 10000);

#else /* !_WIN32 */

	struct timespec ts;
	struct tm       *ptm;
	struct tm       tmbuf;

	clock_gettime(CLOCK_REALTIME, &ts);
	m_msec = static_cast<int>(ts.tv_nsec / 1000000);
	m_epoch = static_cast<int64_t>(ts.tv_sec) * 1000 + m_msec;
	ptm = localtime_r(&ts.tv_sec, &tmbuf);

	m_year = ptm->tm_year + 1900;
	m_month = ptm->tm_mon + 1;
	m_day = ptm->tm_mday;
	m_hour = ptm->tm_hour;
	m_minute = ptm->tm_min;
	m_second = ptm->tm_sec;

#endif
}

int64_t
epoch(unit u)
{
	if (u == unit::second) {
		return time(0);
	} else if (u == unit::millisecond) {
#if defined(_WIN32)

		SYSTEMTIME      st;
		FILETIME        ft;
		ULARGE_INTEGER  current;
		ULARGE_INTEGER  epoch;

		GetLocalTime(&st);
		SystemTimeToFileTime(&st, &ft);
		current.LowPart = ft.dwLowDateTime;
		current.HighPart = ft.dwHighDateTime;
		epoch.QuadPart = 116444736000000000I64;

		return static_cast<int64_t>((current.QuadPart - epoch.QuadPart) / 10000);

#else // !_WIN32

		struct timespec ts;

		clock_gettime(CLOCK_REALTIME, &ts);

		return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;

#endif
	} else {
		return -1;
	}
}

#if defined(_WIN32)

int64_t
file_time_to_epoch(const FILETIME &ft)
{
	int64_t epoch = ft.dwHighDateTime;
	epoch <<= 32;
	epoch |= ft.dwLowDateTime;
	epoch -= 116444736000000000L;
	if (epoch < 0L) epoch = 0L;
	return epoch / 10000000;
}

#endif // _WIN32

} // snf
