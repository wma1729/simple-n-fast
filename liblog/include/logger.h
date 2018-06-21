#ifndef _SNF_LOGGER_H_
#define _SNF_LOGGER_H_

#include "logsev.h"
#include "logrec.h"

namespace snf {
namespace log {

/**
 * Base logger. Every logger must override this.
 */
class logger
{
public:
	virtual ~logger() {}
	virtual severity get_severity() const = 0;
	virtual void log(const record &) = 0;
};

/**
 * The default logger when no logger is registered.
 */
class default_logger : public logger
{
public:
	virtual ~default_logger() {}
	severity get_severity() const override { return severity::all; }
	void log(const record &rec);
};

} // namespace log
} // namespace snf

#endif // _SNF_LOGGER_H_
