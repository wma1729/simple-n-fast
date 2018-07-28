#ifndef _SNF_LOGMGR_H_
#define _SNF_LOGMGR_H_

#include <map>
#include <mutex>
#include "logrec.h"
#include "logger.h"

namespace snf {

// forward declaration of snf::json::object
namespace json {
	class object;
}

namespace log {

class rotation;
class retention;

/**
 * Log manager: singleton.
 * Manages all the loggers. Also provides a way to load
 * loggers via configuration file.
 */
class manager
{
private:
	static constexpr int    BUFLEN = 8191;
	pid_t                   m_cached_pid = 0;
	int                     m_next_id = 0;
	logger                  *m_def_logger = nullptr;
	std::map<int, logger *> m_loggers;
	std::mutex              m_lock;

	manager()
	{
	}

	severity get_severity(const snf::json::object &);
	console_logger::destination get_destination(const snf::json::object &);
	rotation *get_rotation(const snf::json::object &);
	retention *get_retention(const snf::json::object &);
	logger *load(const snf::json::object &);

public:
	manager(const manager &) = delete;
	manager(manager &&) = delete;
	manager & operator=(const manager &) = delete;
	manager & operator=(manager &&) = delete;
	~manager();

	static manager & instance()
	{
		static manager mgr;
		return mgr;	
	}

	pid_t get_pid()
	{
		if (m_cached_pid == 0)
			m_cached_pid = getpid();
		return m_cached_pid;
	}

	void load(const std::string &, const std::string &);

	int add_logger(logger *);
	void remove_logger(int);

	void log(const record &);

	void log(severity, const char *, const char *,
		const char *, int, int, const char *, ...);

	void reset();
};

#if defined(_WIN32)

#define ASSERT(EXP, CLS, ERR, FMT, ...) do {                                           \
	if (!(EXP)) {                                                                  \
		snf::log::manager::instance().log(snf::log::severity::error, CLS,      \
			LOCATION, ERR, FMT, __VA_ARGS__);                              \
		abort();                                                               \
	}                                                                              \
} while (0)

#define LOG_SYSERR(CLS, ERR, FMT, ...) do {                                            \
	snf::log::manager::instance().log(snf::log::severity::error, CLS, LOCATION,    \
		ERR, FMT, __VA_ARGS__);                                                \
} while (0)

#define LOG_ERROR(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::error, CLS, LOCATION,    \
		0, FMT, __VA_ARGS__);                                                  \
} while (0)

#define LOG_WARNING(CLS, FMT, ...) do {                                                \
	snf::log::manager::instance().log(snf::log::severity::warning, CLS, LOCATION,  \
		0, FMT, __VA_ARGS__);                                                  \
} while (0)

#define LOG_INFO(CLS, FMT, ...)    do {                                                \
	snf::log::manager::instance().log(snf::log::severity::info, CLS, LOCATION,     \
		0, FMT, __VA_ARGS__);                                                  \
} while (0)

#define LOG_DEBUG(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::debug, CLS, LOCATION,    \
		0, FMT, __VA_ARGS__);                                                  \
} while (0)

#define LOG_TRACE(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::trace, CLS, LOCATION,    \
		0, FMT, __VA_ARGS__);                                                  \
} while (0)

#else // !_WIN32

#define ASSERT(EXP, CLS, ERR, FMT, ...) do {                                           \
	if (!(EXP)) {                                                                  \
		snf::log::manager::instance().log(snf::log::severity::error, CLS,      \
			LOCATION, ERR, FMT, ##__VA_ARGS__);                            \
		abort();                                                               \
	}                                                                              \
} while (0)

#define LOG_SYSERR(CLS, ERR, FMT, ...) do {                                            \
	snf::log::manager::instance().log(snf::log::severity::error, CLS, LOCATION,    \
		ERR, FMT, ##__VA_ARGS__);                                              \
} while (0)

#define LOG_ERROR(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::error, CLS, LOCATION,    \
		0, FMT, ##__VA_ARGS__);                                                \
} while (0)

#define LOG_WARNING(CLS, FMT, ...) do {                                                \
	snf::log::manager::instance().log(snf::log::severity::warning, CLS, LOCATION,  \
		0, FMT, ##__VA_ARGS__);                                                \
} while (0)

#define LOG_INFO(CLS, FMT, ...)    do {                                                \
	snf::log::manager::instance().log(snf::log::severity::info, CLS, LOCATION,     \
		0, FMT, ##__VA_ARGS__);                                                \
} while (0)

#define LOG_DEBUG(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::debug, CLS, LOCATION,    \
		0, FMT, ##__VA_ARGS__);                                                \
} while (0)

#define LOG_TRACE(CLS, FMT, ...)   do {                                                \
	snf::log::manager::instance().log(snf::log::severity::trace, CLS, LOCATION,    \
		0, FMT, ##__VA_ARGS__);                                                \
} while (0)

#endif

} // namespace log
} // namespace snf

#endif // _SNF_LOGMGR_H_
