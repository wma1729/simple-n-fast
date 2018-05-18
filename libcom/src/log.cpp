#include "common.h"
#include "filesystem.h"
#include "log.h"
#include "error.h"

/*
 * Opens the log file.
 *
 * @param [in] lt - local time.
 */
void
FileLogger::open(const snf::local_time &lt)
{
	char lf[MAXPATHLEN + 1];

	if (!snf::fs::exists(logPath.c_str())) {
		if (mkLogPath) {
			if (snf::fs::mkdir(logPath.c_str(), 0755) != 0) {
				return;
			}
		} else {
			return;
		}
	}

	snprintf(lf, MAXPATHLEN, "%s%c%04d%02d%02d.log",
		logPath.c_str(), snf::pathsep(), lt.year(), lt.month(), lt.day());

	logFile = DBG_NEW snf::file(lf, 0022);

	snf::file_open_flags flags;
	flags.o_append = true;
	flags.o_create = true;
	flags.o_sync = true;

	if (logFile->open(flags, 0600) != E_ok) {
		delete logFile;
		logFile = 0;
	} else {
		lastDay = lt.day();
	}

	return;
}

/**
 * Logs the message to the log file. If the file is not opened,
 * it is opened. The DBG message are only logged if verbose is
 * set to true.
 *
 * @param [in] ll     - log level.
 * @param [in] caller - calling function name.
 * @param [in] msg    - log message.
 */
void
FileLogger::log(log_level_t ll, const char *caller, const char *msg)
{
	char             logbuf[LOGBUFLEN + 1];
	snf::local_time  lt;

	if ((ll == DBG) && !verbose) {
		return;
	}

	std::lock_guard<std::mutex> guard(mutex);

	if (logFile == 0) {
		open(lt);
	}

	if (lastDay != lt.day()) {
		delete logFile;
		logFile = 0;
		open(lt);
	}

	if (logFile) {
		int nbytes = snprintf(logbuf, LOGBUFLEN,
				"%s [%u.%u] [%s] [%s] %s\n",
				lt.str().c_str(),
				snf::getpid(),
				snf::gettid(),
				LevelStr(ll),
				caller,
				msg);

		int bWritten;

		logFile->write(logbuf, nbytes, &bWritten);
	}

	return;
}
