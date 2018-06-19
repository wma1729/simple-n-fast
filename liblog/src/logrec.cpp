#include "logrec.h"
#include "logmgr.h"
#include <iomanip>

namespace snf {
namespace log {

/**
 * Log message terminator.
 */
record &
record::endl(record &rec)
{
	snf::log::manager::instance().log(rec);
	return rec;
}

/**
 * Formats the log record for printing. The following format
 * specifiers are supported:
 *
 * %D - Date YYYY/MM/DD
 * %T - Time HH:MM:SS.MSEC
 * %p - pid
 * %t - tid
 * %s - severity
 * %C - context
 * %c - class
 * %F - file
 * %f - function
 * %l - line
 * %m - message
 */
std::string
record::format(const char *fmt) const
{
	std::ostringstream oss;
	const char *ptr = fmt;

	while (ptr && *ptr) {
		if (*ptr == '%') {
			ptr++;
			switch (*ptr) {
				case '%':
					oss << '%';
					break;

				case 'D':
					oss << std::setfill('0')
						<< std::setw(4) << m_timestamp.year() << "/"
						<< std::setw(2) << m_timestamp.month() << "/"
						<< std::setw(2) << m_timestamp.day();
					break;

				case 'T':
					oss << std::setfill('0')
						<< std::setw(2) << m_timestamp.hour() << ":"
						<< std::setw(2) << m_timestamp.minute() << ":"
						<< std::setw(2) << m_timestamp.second() << "."
						<< std::setw(3) << m_timestamp.millisecond();
					break;

				case 'p':
					oss << m_pid;
					break;

				case 't':
					oss << m_tid;
					break;

				case 's':
					oss << severity_string(m_severity);
					break;

				case 'C':
					if (m_context.empty())
						oss << "no-context";
					else
						oss << m_context;
					break;

				case 'c':
					if (m_class.empty())
						oss << "no-class";
					else
						oss << m_class;
					break;

				case 'F':
					if (m_file.empty())
						oss << "no-file";
					else
						oss << m_file;
					break;

				case 'f':
					if (m_function.empty())
						oss << "no-function";
					else
						oss << m_function;
					break;

				case 'l':
					oss << m_lineno;
					break;

				case 'm':
					oss << m_text.str();
					break;

				default:
					oss << '%' << *ptr;
					break;
			}
		} else {
			oss << *ptr;
		}
		ptr++;
	}

	return oss.str();
}

} // namespace log
} // namespace snf
