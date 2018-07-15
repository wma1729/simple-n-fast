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
 * The console logger.
 */
class console_logger : public logger
{
public:
	enum class destination {
		out, // to standard output
		err, // to standard error
		var  // depends on message severity
	};

private:
	destination m_dest;

public:
	console_logger(
		severity sev = severity::all,
		const std::string &fmt = "[%s] [%f] %m")
		: logger(sev, fmt)
		, m_dest(destination::var)
	{
	}

	virtual ~console_logger() {}

	type get_type() const { return logger::type::console; }

	destination get_destination() const { return m_dest; }
	void set_destination(destination dest) { m_dest = dest; }

	void log(const record &rec);
};

} // namespace log
} // namespace snf

#endif // _SNF_LOGGER_H_
