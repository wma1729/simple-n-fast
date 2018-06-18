#ifndef _SNF_LOGGER_H_
#define _SNF_LOGGER_H_

#include "logrec.h"
#include "logsev.h"

namespace snf {
namespace log {

class logger
{
public:
	virtual bool is_default() const = 0;
	virtual severity get_severity() const = 0;
	virtual void log(const record &) = 0;
};

} // namespace log
} // namespace snf

#endif // _SNF_LOGGER_H_
