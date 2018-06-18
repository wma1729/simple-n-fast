#ifndef _SNF_LOGSEV_H_
#define _SNF_LOGSEV_H_

namespace snf {
namespace log {

/**
 * Logging severity.
 */
enum class severity
{
	ALL,
	TRACE,
	DEBUG,
	INFO,
	WARNING,
	ERROR
};

/**
 * Log severity to string representation.
 */
constexpr const char *
severity_string(severity sev)
{
	switch (sev)
	{
		case severity::TRACE:   return "TRC";
		case severity::DEBUG:   return "DBG";
		case severity::INFO:    return "INF";
		case severity::WARNING: return "WRN";
		case severity::ERROR:   return "ERR";
		default:                return "UNK";
	}
}

} // namespace log
} // namespace snf

#endif // _SNF_LOGSEV_H_
