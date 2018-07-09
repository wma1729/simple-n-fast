#include "logsev.h"
#include "logmgr.h"
#include "logger.h"

#include <cstdarg>

namespace snf {
namespace log {

manager::~manager()
{
	delete m_def_logger;

	std::lock_guard<std::mutex> g(m_lock);

	std::map<int, logger *>::const_iterator I;
	for (I = m_loggers.begin(); I != m_loggers.end(); ++I)
		delete I->second;
}

int
manager::add_logger(logger *l)
{
	std::lock_guard<std::mutex> g(m_lock);
	int id = m_next_id++;
	m_loggers.insert(std::make_pair(id, l));
	return id;
}

void
manager::remove_logger(int id)
{
	std::lock_guard<std::mutex> g(m_lock);
	m_loggers.erase(id);
}

void
manager::log(const record &rec)
{
	int logger_cnt = 0;

	{
		std::lock_guard<std::mutex> g(m_lock);
		std::map<int, logger *>::const_iterator I;
		for (I = m_loggers.begin(); I != m_loggers.end(); ++I) {
			logger_cnt++;
			I->second->log(rec);
		}
	}

	if (logger_cnt == 0) {
		if (m_def_logger == nullptr) {
			m_def_logger = DBG_NEW console_logger;
		}

		m_def_logger->log(rec);
	} else if (m_def_logger) {
		delete m_def_logger;
		m_def_logger = nullptr;
	}
}

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

		rec << buf;
	}

	rec << record::endl;
}

} // namespace log
} // namespace snf
