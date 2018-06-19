#ifndef _SNF_LOGMGR_H_
#define _SNF_LOGMGR_H_

#include <map>
#include <mutex>
#include "logrec.h"
#include "logger.h"

namespace snf {
namespace log {

class manager
{
private:
	static constexpr int    BUFLEN = 8191;
	int                     m_next_id = 0;
	logger                  *m_def_logger = nullptr;
	std::map<int, logger *> m_loggers;
	std::mutex              m_lock;

	manager()
	{
	}

public:
	manager(const manager &) = delete;
	manager(manager &&) = delete;
	manager & operator=(const manager &) = delete;
	manager & operator=(manager &&) = delete;

	static manager & instance()
	{
		static manager mgr;
		return mgr;	
	}

	int add_logger(logger *);
	void remove_logger(int);

	void log(const record &);

	void log(severity, const char *, const char *, const char *,
		const char *, int, const char *, ...);
};

#if !defined(_WIN32)

#define LOG_ERROR(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::error, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_WARNING(CTX, CLS, FMT, ...) do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::warning, (CTX), (CLS), (LOCATION),\
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_INFO(CTX, CLS, FMT, ...)    do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::info, (CTX), (CLS), (LOCATION),   \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_DEBUG(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::debug, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_TRACE(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::trace, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#else // _WIN32

#define LOG_ERROR(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::error, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_WARNING(CTX, CLS, FMT, ...) do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::warning, (CTX), (CLS), (LOCATION),\
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_INFO(CTX, CLS, FMT, ...)    do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::info, (CTX), (CLS), (LOCATION),   \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_DEBUG(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::debug, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_TRACE(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::trace, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#endif

} // namespace log
} // namespace snf

#endif // _SNF_LOGMGR_H_
