#include "common.h"
#include "log.h"

Logger	*TheLogger = 0;
bool    TheVerbosity = false;

/**
 * Logs to either the logger if it is not NULL or to the
 * standard error. DBG messages are only logged if the
 * program is running in verbose mode i.e. TheVerbosity
 * is set to true.
 *
 * @param [in] ll     - log level.
 * @param [in] caller - calling function name.
 * @param [in] fmt    - log message format.
 */
void
Log(log_level_t ll, const char *caller, const char *fmt, ...)
{
	va_list ap;
	char    buf[Logger::LOGBUFLEN + 1];

	if ((ll == DBG) && !TheVerbosity) {
		return;
	}

	va_start(ap, fmt);
	int n = vsnprintf(buf, Logger::LOGBUFLEN, fmt, ap);
	buf[n] = '\0';
	va_end(ap);

	if (TheLogger) {
		TheLogger->log(ll, caller, buf);
	} else if (ll != DBG) {
		fprintf(stderr, "[%s] [%s] %s\n", LevelStr(ll), caller, buf);
	}

	return;
}

/**
 * Logs to either the logger if it is not NULL or to the
 * standard error. DBG messages are only logged if the
 * program is running in verbose mode i.e. TheVerbosity
 * is set to true. This message appends system error string
 * to the log message.
 *
 * @param [in] ll     - log level.
 * @param [in] caller - calling function name.
 * @param [in] error  - OS error.
 * @param [in] fmt    - log message format.
 */
void
Log(log_level_t ll, const char *caller, int error, const char *fmt, ...)
{
	va_list ap;
	char    buf[Logger::LOGBUFLEN + 1];
	char    errbuf1[ERRSTRLEN + 1];
	char    errbuf2[ERRSTRLEN + 1];

	if ((ll == DBG) && !TheVerbosity) {
		return;
	}

	va_start(ap, fmt);
	int n = vsnprintf(buf, Logger::LOGBUFLEN - ERRSTRLEN, fmt, ap);
	buf[n] = '\0';
	va_end(ap);

	snprintf(errbuf2, sizeof(errbuf2), ": %s (%d)",
		GetErrorStr(errbuf1, sizeof(errbuf1), error),
		error);

	strcat(buf, errbuf2);

	if (TheLogger) {
		TheLogger->log(ll, caller, buf);
	} else if (ll != DBG) {
		fprintf(stderr, "[%s] [%s] %s\n", LevelStr(ll), caller, buf);
	}

	return;
}

/**
 * Logs the message and aborts if the condition is not true.
 *
 * @param [in] condition - condition to evaluate.
 * @param [in] file      - file name.
 * @param [in] line      - line number.
 * @param [in] fmt       - log message format.
 */
void
Assert(bool condition, const char *file, int line, const char *fmt, ...)
{
	if (!condition) {
		va_list ap;
		char    buf[Logger::LOGBUFLEN + 1];

		va_start(ap, fmt);
		int n = vsnprintf(buf, Logger::LOGBUFLEN, fmt, ap);
		buf[n] = '\0';
		va_end(ap);

		Log(ERR, "Assert", "condition failed at %s.%d - %s", file, line, buf);

		abort();
	}
}

/**
 * Logs the message and aborts if the condition is not true.
 * This message appends system error string to the log message.
 *
 * @param [in] condition - condition to evaluate.
 * @param [in] file      - file name.
 * @param [in] line      - line number.
 * @param [in] error     - OS error.
 * @param [in] fmt       - log message format.
 */
void
Assert(bool condition, const char *file, int line, int error, const char *fmt, ...)
{
	if (!condition) {
		va_list	ap;
		char    buf[Logger::LOGBUFLEN + 1];

		va_start(ap, fmt);
		int n = vsnprintf(buf, Logger::LOGBUFLEN, fmt, ap);
		buf[n] = '\0';
		va_end(ap);

		Log(ERR, "Assert", error, "condition failed at %s.%d - %s", buf);

		abort();
	}
}
