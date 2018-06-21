#ifndef _SNF_LOGSEV_H_
#define _SNF_LOGSEV_H_

#include <iostream>

namespace snf {
namespace log {

/**
 * Logging severity.
 */
enum class severity
{
	all,
	trace,
	debug,
	info,
	warning,
	error
};

/**
 * Log severity to string representation.
 */
constexpr const char *
severity_string(severity sev)
{
	switch (sev)
	{
		case severity::trace:   return "TRC";
		case severity::debug:   return "DBG";
		case severity::info:    return "INF";
		case severity::warning: return "WRN";
		case severity::error:   return "ERR";
		default:                return "UNK";
	}
}

inline std::ostream &
operator<< (std::ostream &os, severity sev)
{
	os << severity_string(sev);
	return os;
}

} // namespace log
} // namespace snf

#endif // _SNF_LOGSEV_H_