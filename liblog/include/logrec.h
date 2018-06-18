#ifndef _SNF_LOGREC_H_
#define _SNF_LOGREC_H_

#include <string>
#include <iostream>
#include "logsev.h"
#include "logmgr.h"
#include "common.h"
#include "timeutil.h"

namespace snf {
namespace log {

/**
 * A single log message.
 */
class record
{
private:
	std::string         m_context;
	std::string         m_class;
	std::string         m_file;
	std::string         m_function;
	int                 m_lineno;
	snf::local_time     m_timestamp;
	pid_t               m_pid;
	tid_t               m_tid;
	severity            m_severity;
	std::ostringstream  m_text;

public:
	record(const char *ctx, const char *cls,
		const char *file, const char *fcn,
		int line, severity sev)
		: m_lineno(line)
		, m_pid(getpid())
		, m_tid(gettid())
		, m_severity(sev)
	{
		if ((ctx != 0) && (*ctx != '\0'))
			m_context = ctx;
		if ((cls != 0) && (*cls != '\0'))
			m_class = cls;
		if ((file != 0) && (*file != '\0'))
			m_file = file;
		if ((fcn != 0) && (*fcn != '\0'))
			m_function = fcn;
	}

	severity get_severity() const { return m_severity; }

	std::string format(const char *) const;

	static record & endl(record &rec)
	{
		snf::log::manager::instance().log(rec);
		return rec;
	}

	template<typename T>
	record & operator<< (const T t)
	{
		m_text << t;
		return *this;
	}
};

#define LOCATION		__FILE__, __func__, __LINE__

#define ERROR_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::ERROR       \
				}

#define WARNING_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::WARNING     \
				}

#define INFO_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::INFO        \
				}

#define DEBUG_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::DEBUG       \
				}

#define TRACE_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::TRACE       \
				}

/**
 * Should the message be logged?
 */
inline bool
should_log(const record &rec, severity sev)
{
	return (static_cast<int>(rec.get_severity()) >= static_cast<int>(sev));
}

} // namespace log
} // namespace snf

#endif // _SNF_LOGREC_H_
