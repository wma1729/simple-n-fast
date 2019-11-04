#include "logrec.h"
#include "logmgr.h"
#include "json.h"
#include <iomanip>

namespace snf {
namespace log {

/**
 * Initialize the log record.
 * @param cls   [in] - the class name.
 *                     Use 'no-class' if nullptr is used.
 * @param file  [in] - the file name, usually __FILE__.
 *                     Use 'no-file' if nullptr is used.
 * @param fnc   [in] - the function name, usually __func__.
 *                     Use 'no-function' if nullptr is used.
 * @param line  [in] - the line number, usually __LINE__
 * @param error [in] - the system error number: errno/GetLastError()
 * @param sev   [in] - log severity
 */
void
record::init(const char *cls, const char *file, const char *fcn, int line,
	int error, severity sev)
{
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

	m_lineno = line;
	m_error = error;
	m_pid = manager::instance().get_pid();
	m_tid = gettid();
	m_severity = sev;
}

/**
 * Log message terminator.
 * Terminates and actually logs the message.
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
 * json - Log record in json format (OR)
 * json-pretty - Log record in pretty json format (OR)
 * a combination of the following:
 * %D - Date YYYY/MM/DD
 * %T - Time HH:MM:SS.MSEC
 * %p - pid
 * %t - tid
 * %s - severity
 * %c - class
 * %F - file
 * %f - function
 * %l - line
 * %m - message
 *
 * @returns the formatted log record, ready to be logged.
 */
std::string
record::format(const std::string &fmt) const
{
	if (snf::streq(fmt, "json-pretty")) {
		return str(true);
	}

	if (snf::streq(fmt, "json")) {
		return str(false);
	}

	std::ostringstream oss;
	size_t i = 0;

	while (i < fmt.length()) {
		if (fmt[i] == '%') {
			++i;
			switch (fmt[i]) {
				case '%':
					oss << '%';
					break;

				case 'D':
					oss << std::setfill('0')
						<< std::setw(4) << m_timestamp.get_year() << "/"
						<< std::setw(2) << static_cast<int>(m_timestamp.get_month()) << "/"
						<< std::setw(2) << m_timestamp.get_day();
					break;

				case 'T':
					oss << std::setfill('0')
						<< std::setw(2) << m_timestamp.get_hour() << ":"
						<< std::setw(2) << m_timestamp.get_minute() << ":"
						<< std::setw(2) << m_timestamp.get_second() << "."
						<< std::setw(3) << m_timestamp.get_millisecond();
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
					if (m_error != 0) {
						char errstr[ERRSTRLEN + 1];
						oss << ": " << syserr(errstr, ERRSTRLEN, m_error)
							<< " (" << m_error << ")";
					}
					break;

				default:
					oss << '%' << fmt[i];
					break;
			}
		} else {
			oss << fmt[i];
		}
		++i;
	}

	return oss.str();
}

/**
 * Converts log record to json representation.
 */
std::string
record::str(bool pretty) const
{
	snf::json::value v = snf::json::object {
		std::make_pair("class", m_class),
		std::make_pair("file", m_file),
		std::make_pair("function", m_function),
		std::make_pair("lineno", m_lineno),
		std::make_pair("error", m_error),
		std::make_pair("timestamp", m_timestamp.epoch(unit::millisecond)),
		std::make_pair("pid", m_pid),
		std::make_pair("tid", m_tid),
		std::make_pair("severity", severity_string(m_severity)),
		std::make_pair("text", m_text.str())
	};

	return v.str(pretty);
}

} // namespace log
} // namespace snf
