#include "logrec.h"
#include "logmgr.h"
#include <iomanip>

namespace snf {
namespace log {

record::record(const char *ctx, const char *cls,
	const char *file, const char *fcn,
	int line, severity sev)
	: m_lineno(line)
	, m_pid(manager::instance().get_pid())
	, m_tid(gettid())
	, m_severity(sev)
{
	if (ctx && *ctx)
		m_context = ctx;
	else
		m_context = "no-context";

	if (cls && *cls)
		m_class = cls;
	else
		m_class = "no-class";

	if (file && *file)
		m_file = file;
	else
		m_file = "no-file";

	if (fcn && *fcn)
		m_function = fcn;
	else
		m_function = "no-function";
}

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
					oss << m_context;
					break;

				case 'c':
					oss << m_class;
					break;

				case 'F':
					oss << m_file;
					break;

				case 'f':
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
