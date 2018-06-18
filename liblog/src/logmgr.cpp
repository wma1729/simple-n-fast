#include "logsev.h"
#include "logrec.h"

#include <cstdarg>

namespace snf {
namespace log {

void
manager::log(severity sev, const char *ctx, const char *cls,
	const char *file, const char *fcn, int line, const char *fmt, ...)
{
	record rec { ctx, cls, file, fcn, line, sev };

	if (fmt && *fmt) {
		char buf[BUFLEN + 1];
		va_list args;

		va_start(args, fmt);
		int n = vsnprintf(buf, BUFLEN, fmt, args);
		buf[n] = '\0';
		va_end(args);

		record rec { ctx, cls, file, fcn, line, sev };
		rec << buf;
	}

	rec << record::endl;
}

} // namespace log
} // namespace snf
