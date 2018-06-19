#ifndef _SNF_LOGREC_H_
#define _SNF_LOGREC_H_

#include <string>
#include <iostream>
#include "logsev.h"
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

	using record_terminator = record &(record &);

public:
	record(const char *ctx, const char *cls,
		const char *file, const char *fcn,
		int line, severity sev)
		: m_lineno(line)
		, m_pid(getpid())
		, m_tid(gettid())
		, m_severity(sev)
	{
		if (ctx && *ctx)
			m_context = ctx;
		if (cls && *cls)
			m_class = cls;
		if (file && *file)
			m_file = file;
		if (fcn && *fcn)
			m_function = fcn;
	}

	severity get_severity() const { return m_severity; }

	std::string format(const char *) const;

	static record & endl(record &);

	record & operator<< (record_terminator terminator) { return terminator(*this); }

	template<typename T>
	record & operator<< (const T t) { m_text << t; return *this; }
};

#define LOCATION		__FILE__, __func__, __LINE__

#define ERROR_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::error       \
				}

#define WARNING_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::warning     \
				}

#define INFO_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::info        \
				}

#define DEBUG_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::debug       \
				}

#define TRACE_STRM(CTX, CLS)	snf::log::record {                      \
					CTX,                            \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::trace       \
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
