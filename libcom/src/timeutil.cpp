#include "timeutil.h"
#include <ctime>

namespace snf {

datetime::datetime(bool utc)
	: m_utc(utc)
	, m_recalc(false)
{
#if defined(_WIN32)

	SYSTEMTIME      st;
	FILETIME        ft;
	ULARGE_INTEGER  current;
	ULARGE_INTEGER  epoch;

	if (m_utc)
		GetSystemTime(&st);
	else
		GetLocalTime(&st);

	m_tm.tm_year = st.wYear - 1900;
	m_tm.tm_mon = st.wMonth - 1;
	m_tm.tm_mday = st.wDay;
	m_tm.tm_hour = st.wHour;
	m_tm.tm_min = st.wMinute;
	m_tm.tm_sec = st.wSecond;
	m_tm.tm_wday = st.wDayOfWeek;
	m_msec = st.wMilliseconds;

	SystemTimeToFileTime(&st, &ft);
	current.LowPart = ft.dwLowDateTime;
	current.HighPart = ft.dwHighDateTime;
	epoch.QuadPart = 116444736000000000I64;
	m_epoch = static_cast<int64_t>((current.QuadPart - epoch.QuadPart) / 10000000);

#else /* !_WIN32 */

	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	m_msec = static_cast<int>(ts.tv_nsec / 1000000);
	m_epoch = static_cast<int64_t>(ts.tv_sec);

	if (m_utc)
		gmtime_r(&ts.tv_sec, &m_tm);
	else
		localtime_r(&ts.tv_sec, &m_tm);

#endif
}

datetime::datetime(int64_t epoch, unit u, bool utc)
	: m_utc(utc)
	, m_recalc(false)
{
	if (u == unit::millisecond) {
		m_epoch = epoch / 1000;
		m_msec = epoch % 1000;
	} else {
		m_epoch = epoch;
		m_msec = 0;
	}

#if defined(_WIN32)

	SYSTEMTIME      st;
	FILETIME        ft;

	epoch_to_file_time(ft, m_epoch);
	if (m_utc) {
		FileTimeToSystemTime(&ft, &st);
	} else {
		FILETIME lft;
		FileTimeToLocalFileTime(&ft, &lft);
		FileTimeToSystemTime(&lft, &st);
	}

	m_tm.tm_year = st.wYear - 1900;
	m_tm.tm_mon = st.wMonth - 1;
	m_tm.tm_mday = st.wDay;
	m_tm.tm_hour = st.wHour;
	m_tm.tm_min = st.wMinute;
	m_tm.tm_sec = st.wSecond;
	m_tm.tm_wday = st.wDayOfWeek;

#else /* !_WIN32 */

	if (m_utc)
		gmtime_r(&m_epoch, &m_tm);
	else
		localtime_r(&m_epoch, &m_tm);

#endif
}

void
datetime::recalculate()
{
	if (m_recalc) {

#if defined(_WIN32)

		SYSTEMTIME      st;
		FILETIME        ft;
		ULARGE_INTEGER  current;
		ULARGE_INTEGER  epoch;

		st.wYear = m_tm.tm_year + 1900;
		st.wMonth = m_tm.tm_mon + 1;
		st.wDay = m_tm.tm_mday;
		st.wHour = m_tm.tm_hour;
		st.wMinute = m_tm.tm_min;
		st.wSecond = m_tm.tm_sec;
		st.wDayOfWeek = m_tm.tm_wday;
		st.wMilliseconds = m_msec;

		SystemTimeToFileTime(&st, &ft);
		if (!m_utc) {
			FILETIME utcft;
			LocalFileTimeToFileTime(&ft, &utcft);
			ft = utcft;
		}

		current.LowPart = ft.dwLowDateTime;
		current.HighPart = ft.dwHighDateTime;
		epoch.QuadPart = 116444736000000000I64;
		m_epoch = static_cast<int64_t>((current.QuadPart - epoch.QuadPart) / 10000000);

#else
		if (m_utc) {
			m_epoch = 0;

			int year = m_tm.tm_year + 1900;
			bool leap_year = is_leap_year(year);

			for (int y = 1970; y < year; ++y)
				m_epoch += seconds_in_year(is_leap_year(y));

			for (int m = 0; m < m_tm.tm_mon; ++m)
				m_epoch += seconds_in_month(m, leap_year);

			m_epoch += (m_tm.tm_mday - 1) * seconds_in_day;
			m_epoch += m_tm.tm_hour * seconds_in_hour;
			m_epoch += m_tm.tm_min * seconds_in_minute;
			m_epoch += m_tm.tm_sec;
		} else {
			m_epoch = mktime(&m_tm);
		}
#endif
		m_recalc = false;
	}
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

		GetSystemTime(&st);
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

void
epoch_to_file_time(FILETIME &ft, int64_t epoch)
{
	LONGLONG ll = Int32x32To64(epoch, 10000000) + 116444736000000000L;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = ll >> 32;
}

#endif // _WIN32

} // snf
