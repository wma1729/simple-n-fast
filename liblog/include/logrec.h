#ifndef _SNF_LOGREC_H_
#define _SNF_LOGREC_H_

#include <string>
#include <sstream>
#include <iostream>
#include "logsev.h"
#include "common.h"
#include "timeutil.h"

namespace snf {
namespace log {

/**
 * A single log record.
 */
class record
{
private:
	std::string         m_class;        // class name (user specified)
	std::string         m_file;         // file name, __FILE__ 
	std::string         m_function;     // function name, __func__
	int                 m_lineno;       // line number, __LINE__
	int                 m_error;        // system error (user specified)
	snf::datetime       m_timestamp;    // local time stamp
	pid_t               m_pid;          // process ID
	tid_t               m_tid;          // thread ID
	severity            m_severity;     // severity (user specified)
	std::ostringstream  m_text;         // actual log text (user specified)

	// Pointer to function (for endl implementation)
	using record_terminator = record &(record &);

	void init(const char *, const char *, const char *, int, int, severity);

public:
	// Static function of type record_terminator
	static record & endl(record &);

	record(const char *file, const char *func, int line, severity sev)
	{
		init(nullptr, file, func, line, 0, sev);
	}

	record(const char *clz, const char *file, const char *func, int line, severity sev)
	{
		init(clz, file, func, line, 0, sev);
	}

	record(const char *file, const char *func, int line, int err, severity sev)
	{
		init(nullptr, file, func, line, err, sev);
	}

	record(const char *clz, const char *file, const char *func, int line, int err, severity sev)
	{
		init(clz, file, func, line, err, sev);
	}

	~record() {}

	severity get_severity() const { return m_severity; }
	snf::datetime get_timestamp() const { return m_timestamp; }

	record & operator<< (record_terminator terminator) { return terminator(*this); }
	template<typename T> record & operator<< (const T &t) { m_text << t; return *this; }

	std::string format(const std::string &) const;
	std::string str(bool pretty = false) const;
};

#define LOCATION		__FILE__, __func__, __LINE__

/*
 * All the macros ending with _STRM takes at least one paramaters
 * which is the class name. This is user specified. There is no
 * portable way (or at least I am not aware of it) to fetch this
 * automatically. ERROR_STRM and WARNING_STRM can take one
 * additional parameter which is the errno/GetLastError().
 * Do not use macros ending with number 1 or 2 directly.
 */

#define ERROR_STRM2(CLS, ERR)	snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					ERR,                            \
					snf::log::severity::error       \
				}

#define ERROR_STRM1(CLS)	snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::error       \
				}

#define ERROR_STRM(...)		CALL_MACRO(ERROR_STRM, __VA_ARGS__)

#define WARNING_STRM2(CLS, ERR)	snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					ERR,                            \
					snf::log::severity::warning     \
				}

#define WARNING_STRM1(CLS)	snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::warning     \
				}

#define WARNING_STRM(...)	CALL_MACRO(WARNING_STRM, __VA_ARGS__)

#define INFO_STRM(CLS)		snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::info        \
				}

#define DEBUG_STRM(CLS)		snf::log::record {                      \
					CLS,                            \
					LOCATION,                       \
					snf::log::severity::debug       \
				}

#define TRACE_STRM(CLS)		snf::log::record {                      \
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
