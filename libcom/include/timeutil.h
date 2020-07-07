#ifndef _SNF_TIMEUTIL_H_
#define _SNF_TIMEUTIL_H_

#include "common.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace snf {

enum class unit {
	millisecond,
	second
};

enum class day {
	sunday = 0,
	monday,
	tuesday,
	wednesday,
	thursday,
	friday,
	saturday
};

const std::string days[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

enum class month {
	january = 1,
	february,
	march,
	april,
	may,
	june,
	july,
	august,
	september,
	october,
	november,
	december
};

const std::string months[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};
	
constexpr int seconds_in_minute = 60;
constexpr int seconds_in_hour = 3600;
constexpr int seconds_in_day = 86400;

/*
 * From Wikipedia:
 * Every year that is exactly divisible by four is a leap year,
 * except for years that are exactly divisible by 100,
 * but these centurial years are leap years if they are exactly divisible by 400.
 * For example, the years 1700, 1800, and 1900 are not leap years,
 * but the years 1600 and 2000 are.
 */
inline bool is_leap_year(int year)
{
	if ((year % 4) != 0)
		return false;
	else if ((year % 100) != 0)
		return true;
	else if ((year % 400) != 0)
		return false;
	else
		return true;
}

constexpr int days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

constexpr int64_t seconds_in_month(int m, bool leap_year)
{
	return (((m == 1) && leap_year) ? 29 : days_in_month[m]) * seconds_in_day;
}

constexpr int64_t seconds_in_year(bool leap_year)
{
	return (leap_year ? 366 : 365) * seconds_in_day;
}

enum time_format
{
	common, // YYYY/MM/DD hh:mm:ss.mse
	imf	// Internet Message format
};

int64_t epoch(unit u = unit::second);

/**
 * Date and time. Hides platform variations.
 */
class datetime
{
private:
	std::tm m_tm;     // Native date time
	int     m_msec;   // milli-second: 0 - 999
	int64_t m_epoch;  // seconds since epoch
	bool    m_utc;    // Is UTC time?
	bool    m_recalc; // Re-calculate epoch time

public:
	datetime(bool utc = false);
	datetime(int64_t, unit u = unit::second, bool utc = true);
	datetime(const datetime &dt)
		: m_tm(dt.m_tm)
		, m_msec(dt.m_msec)
		, m_epoch(dt.m_epoch)
		, m_utc(dt.m_utc)
		, m_recalc(dt.m_recalc)
	{
	}

	static datetime get(const std::string &, time_format fmt = time_format::common, bool utc = false);

	int   get_year() const          { return m_tm.tm_year + 1900; }
	void  set_year(int y)           { m_tm.tm_year = y - 1900; m_recalc = true; }
	month get_month() const         { return static_cast<month>(m_tm.tm_mon + 1); }
	void  set_month(month m)        { m_tm.tm_mon = static_cast<int>(m) - 1; m_recalc = true; }
	int   get_day() const           { return m_tm.tm_mday; }
	void  set_day(int d)            { m_tm.tm_mday = d; m_recalc = true; }
	int   get_hour() const          { return m_tm.tm_hour; }
	void  set_hour(int h)           { m_tm.tm_hour = h; m_recalc = true; }
	int   get_minute() const        { return m_tm.tm_min; }
	void  set_minute(int m)         { m_tm.tm_min = m; m_recalc = true; }
	int   get_second() const        { return m_tm.tm_sec; }
	void  set_second(int s)         { m_tm.tm_sec = s; m_recalc = true; }
	int   get_millisecond() const   { return m_msec; }
	void  set_millisecond(int ms)   { m_msec = ms; m_recalc = true; }
	day   get_day_of_week() const   { return static_cast<day>(m_tm.tm_wday); }
	void  set_day_of_week(day d)    { m_tm.tm_wday = static_cast<int>(d); m_recalc = true; }

	int64_t epoch(unit u = unit::second) const
	{
		return (u == unit::millisecond) ? m_epoch * 1000 + m_msec : m_epoch;
	}

	const std::tm *native() const { return &m_tm; }

	void recalculate();

	std::string str(time_format fmt = time_format::common) const;
};

#if defined(_WIN32)
int64_t file_time_to_epoch(const FILETIME &);
void epoch_to_file_time(FILETIME &, int64_t);
#endif

} // snf

#endif // _SNF_TIMEUTIL_H_
