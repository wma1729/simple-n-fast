#ifndef _SNF_LOGGER_H_
#define _SNF_LOGGER_H_

#include <string>
#include "logsev.h"
#include "logrec.h"

namespace snf {
namespace log {

/**
 * Base logger. Every logger must override this.
 */
class logger
{
private:
	severity    m_sev;
	std::string m_fmt;

public:
	enum class type { console, file };

	/**
	 * Initializes logger object.
	 * @param [in] sev - log severity
	 * @param [in] fmt - log format.
	 */
	logger(severity sev, const std::string &fmt)
		: m_sev(sev)
		, m_fmt(fmt)
	{
	}

	virtual ~logger() {}

	virtual type get_type() const = 0;

	virtual severity get_severity() const { return m_sev; }
	virtual void set_severity(severity sev) { m_sev = sev; }

	const std::string & get_format() const { return m_fmt; }
	void set_format(const std::string &fmt) { m_fmt = fmt; }

	virtual void log(const record &) = 0;
};

/**
 * The console logger. Supports only a few attributes:
 * - type (console)
 * - severity (valid log severity)
 * - format (logging format)
 * - destination (error and warning messages go to stderr,
 *   and the remaining ones go to stdout by default).
 */
class console_logger : public logger
{
public:
	static constexpr const char *default_format = "[%s] [%f] %m";

	enum class destination {
		out, // everything to standard output
		err, // everything to standard error
		var  // depends on message severity (default)
	};

	console_logger(
		severity sev = severity::all,
		const std::string &fmt = default_format)
		: logger(sev, fmt)
		, m_dest(destination::var)
	{
	}

	virtual ~console_logger() {}

	type get_type() const { return logger::type::console; }

	destination get_destination() const { return m_dest; }
	void set_destination(destination dest) { m_dest = dest; }

	void log(const record &rec);

private:
	destination m_dest;

};

} // namespace log
} // namespace snf

#endif // _SNF_LOGGER_H_
