#ifndef _SNF_LOGMGR_H_
#define _SNF_LOGMGR_H_

namespace snf {
namespace log {

class record;

class manager
{
private:
	static constexpr int BUFLEN = 8191;

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

	void log(const record &);

	void log(severity, const char *, const char *, const char *,
		const char *, int, const char *, ...);
};

#if !defined(_WIN32)

#define LOG_ERROR(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::ERROR, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_WARNING(CTX, CLS, FMT, ...) do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::WARNING, (CTX), (CLS), (LOCATION),\
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_INFO(CTX, CLS, FMT, ...)    do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::INFO, (CTX), (CLS), (LOCATION),   \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_DEBUG(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::DEBUG, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#define LOG_TRACE(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::TRACE, (CTX), (CLS), (LOCATION),  \
		(FMT), __VA_ARGS__);                                                            \
} while (0)

#else // _WIN32

#define LOG_ERROR(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::ERROR, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_WARNING(CTX, CLS, FMT, ...) do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::WARNING, (CTX), (CLS), (LOCATION),\
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_INFO(CTX, CLS, FMT, ...)    do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::INFO, (CTX), (CLS), (LOCATION),   \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_DEBUG(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::DEBUG, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#define LOG_TRACE(CTX, CLS, FMT, ...)   do {                                                    \
	snf::log::manager::instance().log(snf::log::severity::TRACE, (CTX), (CLS), (LOCATION),  \
		(FMT), ##__VA_ARGS__);                                                          \
} while (0)

#endif

} // namespace log
} // namespace snf

#endif // _SNF_LOGMGR_H_
