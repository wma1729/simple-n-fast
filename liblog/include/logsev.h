#ifndef _SNF_LOGSEV_H_
#define _SNF_LOGSEV_H_

#include "common.h"
#include <iostream>

namespace snf {
namespace log {

/**
 * Logging severity.
 */
enum class severity
{
	all,	// not a real log severity
	trace,
	debug,
	info,
	warning,
	error
};

/**
 * Log severity to string representation.
 */
inline const char *
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

/**
 * String representation to log severity.
 */
inline severity
string_to_severity(const char *s)
{
	if ((s == nullptr) || (*s == '\0'))
		return snf::log::severity::all;

	if ((strcasecmp(s, "trc") == 0) || (strcasecmp(s, "trace") == 0))
		return snf::log::severity::trace;
	else if ((strcasecmp(s, "dbg") == 0) || (strcasecmp(s, "debug") == 0))
		return snf::log::severity::debug;
	else if ((strcasecmp(s, "inf") == 0) || (strcasecmp(s, "info") == 0))
		return snf::log::severity::info;
	else if ((strcasecmp(s, "wrn") == 0) || (strcasecmp(s, "warning") == 0))
		return snf::log::severity::warning;
	else if ((strcasecmp(s, "err") == 0) || (strcasecmp(s, "error") == 0))
		return snf::log::severity::error;

	return snf::log::severity::all;
}

/**
 * Simple stream read operator for log severity.
 */
inline std::ostream &
operator<< (std::ostream &os, severity sev)
{
	os << severity_string(sev);
	return os;
}

} // namespace log
} // namespace snf

#endif // _SNF_LOGSEV_H_
