#ifndef _SNF_DEFAULT_H_
#define _SNF_DEFAULT_H_

#include "logger.h"

namespace snf {
namespace log {

class default_logger : public logger
{
public:
	bool is_default() const override { return true; }
	severity get_severity() const override { return severity::INFO; }
	void log(const record &rec) override;
};

} // namespace log
} // namespace snf

#endif // _SNF_DEFAULT_H_
