#ifndef _SNF_LOG_H_
#define _SNF_LOG_H_

#include "common.h"
#include "file.h"
#include "lock.h"
#include "util.h"

/**
 * Base logger class.
 */
class Logger
{
protected:
	bool verbose;

public:
	static const int LOGBUFLEN = 8191;

	/**
	 * Constructs the Logger object.
	 * @param [in] v - log verbosity.
	 */
	Logger(bool v)
		: verbose(v)
	{
	}

	/**
	 * Virtual destructor. The sub-class should override it.
	 */
	virtual ~Logger()
	{
	}

	virtual void log(log_level_t, const char *, const char *) = 0;

	virtual bool setVerbosity(bool value)
	{
		bool overbose = verbose;
		verbose = value;
		return overbose;
	}

	virtual bool getVerbosity() const
	{
		return verbose;
	}
};

/**
 * File Logger. Logs to a file. The file name is:
 * <pre>
 * logpath + path_separator + YYYYMMDD.log
 * </pre>
 * The file is rotated everyday at mid-night.
 */
class FileLogger : public Logger
{
private:
	std::string logPath;
	bool        mkLogPath;
	int         lastDay;
	File        *logFile;
	Mutex       mutex;

	void open(const local_time_t *);

public:
	/**
	 * Constructs the file logger object.
	 * @param [in] path    - the log path.
	 * @param [in] verbose - the log verbosity.
	 */
	FileLogger(const std::string &path, bool verbose = false)
		: Logger(verbose),
		  logPath(path),
		  mkLogPath(false),
		  lastDay(-1),
		  logFile(0),
		  mutex()
	{
	}

	/**
	 * Destroys the file logger object.
	 */
	~FileLogger()
	{
		if (logFile != 0) {
			delete logFile;
			logFile = 0;
		}
	}

	const char *getLogPath() const
	{
		return logPath.c_str();
	}

	/**
	 * Makes the log path if it does not exist.
	 * By default, the log path is not created. If the caller
	 * wants the log path to be created, they must call this
	 * method before opening the log.
	 */
	void makeLogPath()
	{
		mkLogPath = true;
	}

	void log(log_level_t, const char *, const char *);
};

#endif // _SNF_LOG_H_
