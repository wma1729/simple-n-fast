#include "common.h"
#include "filesystem.h"
#include "log.h"

/*
 * Opens the log file.
 *
 * @param [in] lt - local time.
 */
void
FileLogger::open(const local_time_t *lt)
{
	char lf[MAXPATHLEN + 1];

	if (!FileSystem::exists(logPath.c_str())) {
		if (mkLogPath) {
			if (FileSystem::mkdir(logPath.c_str(), 0755) != 0) {
				return;
			}
		} else {
			return;
		}
	}

	snprintf(lf, MAXPATHLEN, "%s%c%04d%02d%02d.log",
		logPath.c_str(), PATH_SEP, lt->year, lt->month, lt->day);

	logFile = new File(lf, 0022);

	FileOpenFlags flags;
	flags.o_append = true;
	flags.o_create = true;
	flags.o_sync = true;

	if (logFile->open(flags, 0600) != E_ok) {
		delete logFile;
		logFile = 0;
	} else {
		lastDay = lt->day;
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
	char         logbuf[LOGBUFLEN + 1];
	local_time_t lt;

	if ((ll == DBG) && !verbose) {
		return;
	}

	GetLocalTime(&lt);

	if (E_ok == mutex.lock()) {

		if (logFile == 0) {
			open(&lt);
		}

		if (lastDay != lt.day) {
			delete logFile;
			logFile = 0;
			open(&lt);
		}

		if (logFile) {
			int nbytes = snprintf(logbuf, LOGBUFLEN,
					  "%04d/%02d/%02d %02d:%02d:%02d.%03d [%u.%u] [%s] [%s] %s\n",
					  lt.year,
					  lt.month,
					  lt.day,
					  lt.hour,
					  lt.minute,
					  lt.second,
					  lt.msec,
					  getpid(),
					  gettid(),
					  LevelStr(ll),
					  caller,
					  msg);

			int bWritten;

			logFile->write(logbuf, nbytes, &bWritten);
		}

		mutex.unlock();	
	}

	return;
}
