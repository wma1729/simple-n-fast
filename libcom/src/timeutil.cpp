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

	memset(&m_tm, 0, sizeof(m_tm));

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

datetime
datetime::get(const std::string &str, time_format fmt, bool utc)
{
	int  i;
	datetime t((fmt == time_format::imf) ? true : utc);

	if (fmt == time_format::imf) {
		bool found = false;

		std::string s(std::move(str.substr(0, 3)));
		for (i = 0; i < 7; ++i) {
			if (days[i] == s) {
				t.set_day_of_week(static_cast<day>(i));
				found = true;
				break;
			}
		}

		if (!found || (str[3] != ',') || (str[4] != ' '))
			throw std::invalid_argument("invalid date/time");

		i = std::stoi(str.substr(5, 2));
		if ((i < 1) || (i > 31) || (str[7] != ' '))
			throw std::invalid_argument("invalid date/time");
		t.set_day(i);

		found = false;

		s = std::move(str.substr(8, 3));
		for (int i = 0; i < 12; ++i) {
			if (months[i] == s) {
				t.set_month(static_cast<month>(i + 1));
				found = true;
				break;
			}
		}

		if (!found || (str[11] != ' '))
			throw std::invalid_argument("invalid date/time");

		i = std::stoi(str.substr(12, 4));
		if ((i < 0) || (str[16] != ' '))
			throw std::invalid_argument("invalid date/time");
		t.set_year(i);

		i = std::stoi(str.substr(17, 2));
		if ((i < 0) || (i > 23) || (str[19] != ':'))
			throw std::invalid_argument("invalid date/time");
		t.set_hour(i);

		i = std::stoi(str.substr(20, 2));
		if ((i < 0) || (i > 59) || (str[22] != ':'))
			throw std::invalid_argument("invalid date/time");
		t.set_minute(i);

		i = std::stoi(str.substr(23, 2));
		if ((i < 0) || (i > 59) || (str[25] != ' '))
			throw std::invalid_argument("invalid date/time");
		t.set_second(i);

		s = std::move(str.substr(26, 3));
		if (s != "GMT")
			throw std::invalid_argument("invalid date/time");
	} else {
		i = std::stoi(str.substr(0, 4));
		if ((i < 0) || (str[4] != '/'))
			throw std::invalid_argument("invalid date/time");
		t.set_year(i);

		i = std::stoi(str.substr(5, 2)) - 1;
		if ((i < 0) || (i > 11) || (str[7] != '/'))
			throw std::invalid_argument("invalid date/time");
		t.set_month(static_cast<month>(i + 1));

		i = std::stoi(str.substr(8, 2));
		if ((i < 1) || (i > 31) || (str[10] != ' '))
			throw std::invalid_argument("invalid date/time");
		t.set_day(i);

		i = std::stoi(str.substr(11, 2));
		if ((i < 0) || (i > 23) || (str[13] != ':'))
			throw std::invalid_argument("invalid date/time");
		t.set_hour(i);

		i = std::stoi(str.substr(14, 2));
		if ((i < 0) || (i > 59) || (str[16] != ':'))
			throw std::invalid_argument("invalid date/time");
		t.set_minute(i);

		i = std::stoi(str.substr(17, 2));
		if ((i < 0) || (i > 59) || (str[19] != '.'))
			throw std::invalid_argument("invalid date/time");
		t.set_second(i);

		i = std::stoi(str.substr(20, 3));
		if ((i < 0) || (i > 999))
			throw std::invalid_argument("invalid date/time");
		t.set_millisecond(i);
	}

	t. recalculate();

	return t;
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

std::string
datetime::str(time_format fmt) const
{
	std::ostringstream oss;

	if (fmt == time_format::imf) {
		int d = static_cast<int>(get_day_of_week());
		int m = static_cast<int>(get_month()) - 1;

		oss << std::setfill('0')
			<< days[d] << ", "
			<< std::setw(2) << get_day() << " "
			<< months[m] << " "
			<< std::setw(4) << get_year() << " "
			<< std::setw(2) << get_hour() << ":"
			<< std::setw(2) << get_minute() << ":"
			<< std::setw(2) << get_second() << " GMT";
	} else {
		oss << std::setfill('0')
			<< std::setw(4) << get_year() << "/"
			<< std::setw(2) << static_cast<int>(get_month()) << "/"
			<< std::setw(2) << get_day() << " "
			<< std::setw(2) << get_hour() << ":"
			<< std::setw(2) << get_minute() << ":"
			<< std::setw(2) << get_second() << "."
			<< std::setw(3) << get_millisecond();
	}

	return oss.str();
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
