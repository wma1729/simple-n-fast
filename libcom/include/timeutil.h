#ifndef _SNF_TIMEUTIL_H_
#define _SNF_TIMEUTIL_H_

#include <sstream>
#include <iomanip>

namespace snf {

enum class unit { millisecond, second };

int64_t epoch(unit u = unit::second);

/**
 * Local time. Hides platform variations.
 */
class local_time
{
private:
	int m_year;       // 4 digit year
	int m_month;      // month: 1 is January and 12 is December
	int m_day;        // day: 1 - 31
	int m_hour;       // hour: 0 - 23
	int m_minute;     // minute: 0 - 59
	int m_second;     // second: 0 - 59
	int m_msec;       // milli-second: 0 - 999
	int64_t m_epoch;  // milli-seconds since epoch

public:
	local_time();
	local_time(const local_time &lt)
		: m_year(lt.m_year)
		, m_month(lt.m_month)
		, m_day(lt.m_day)
		, m_hour(lt.m_hour)
		, m_minute(lt.m_minute)
		, m_second(lt.m_second)
		, m_msec(lt.m_msec)
		, m_epoch(lt.m_epoch) {}
	local_time(local_time &&) = delete;

	int year()        const { return m_year; }
	int month()       const { return m_month; }
	int day()         const { return m_day; }
	int hour()        const { return m_hour; }
	int minute()      const { return m_minute; }
	int second()      const { return m_second; }
	int millisecond() const { return m_msec; }

	int64_t epoch(unit u) const
	{
		if (u == unit::millisecond)
			return m_epoch;
		else if (u == unit::second)
			return m_epoch / 1000;
		else
			return -1;
	}

	std::string str() const
	{
		std::ostringstream oss;
		oss << std::setfill('0')
			<< std::setw(4) << m_year << "/"
			<< std::setw(2) << m_month << "/"
			<< std::setw(2) << m_day << " "
			<< std::setw(2) << m_hour << ":"
			<< std::setw(2) << m_minute << ":"
			<< std::setw(2) << m_second << "."
			<< std::setw(3) << m_msec;
		return oss.str();
	}
};

} // snf

#endif // _SNF_TIMEUTIL_H_
