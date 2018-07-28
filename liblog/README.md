# liblog

A feature rich and easy to use logging library. It provides the following capabilities:
* Both C++ streaming and conventional printf style formatting interfaces.
* Multiple configurable loggers for the same component (process).
* Thread safety.
* Log rotation schemes: daily or based on size or a combination of both.
* Log retention schemes: how long to retain the logs.
* It may not be fastest library but fast enough for most needs.

### Log severity

Following log severities are supported:
1. trace
2. debug
3. info
4. warning
5. error

The log verbosity is controlled by the severity as well. If the severity is set to *debug*, then
all logs at *debug* and higher are logger. Setting the severity to *warning* will only log *warning*
and *error* message. A good default for production code is *info*. *debug* and *trace* are more
suitable for debugging.

### Log record

All logging interfaces, either the C++ stream based or the conventional printf style formatting,
prepares a log **record** which is eventually logged by all the registered loggers. A log
**record** has the following fields:
* **Class name** Unfortunately there is no portable way to get or deduce the class name automatically. As such the caller is expected to provide this. Where **class name** does not make sense, **nullptr** can be passed.
* **File name** This is obtained from `__FILE__` macro.
* **Function name** This is obtained from `__func__` macro.
* **Line number** This is obtained from `__LINE__` macro.
* **System error code** For *warning* and *error* message, this can be optionally specified by the caller. The logging system uses it to generate the corresponding string and log it (more later).
* **Time stamp** This is the local time stamp with millisecond precision.
* **Process ID** The process ID.
* **Thread ID** The thread ID.
* **Log severity** See above.
* **Log text** The actual log text.

### Log format

The formatting of the log message can be controlled by some formatting keywords or printf style format specifiers. The formatting keywords supported are: *json*
```json
{ "class" : "no-class", "error" : 0, "file" : "logtester.cpp", "function" : "main", "lineno" : 66, "pid" : 195, "severity" : "ERR", "text" : "a sample error message", "tid" : 195, "timestamp" : 1532658086959 }
```
or *json-pretty*
```json
{
  "class" : "no-class",
  "error" : 0,
  "file" : "logtester.cpp",
  "function" : "main",
  "lineno" : 66,
  "pid" : 195,
  "severity" : "ERR",
  "text" : "a sample error message",
  "tid" : 195,
  "timestamp" : 1532658086959
}
```
The following printf style format specifiers are supported:

format specifier | Interpretation
---------------- | --------------
%D  | Date YYYY/MM/DD
%T  | Time hh:mm:ss.msec
%p  | Process ID
%t  | Thread ID
%s  | Log severity
%c  | Class name
%F  | File name
%f  | Function name
%l  | Line number
%m  | Log message/text

The default format for console logger is `[%s] [%f] %m`
```
[ERR] [main] a sample error message
```
and the default format for file logger is `%D %T %p.%t [%s] [%F:%c.%f.%l] %m`
```
2018/07/27 15:05:02.876 192.192 [ERR] [logtester.cpp:no-class.main.66] a sample error message
```

### Log file name format

The formatting of the log file names is also controlled by printf style format specifiers. The supported format specifiers are:

format specifier | Interpretation    | Rotation Requirements
---------------- | ----------------- | ---------------------
%D  | YYYYMMDD  | Must be specified for **daily** rotation.
%N  | 6 digit sequence number starting with 000000 | Must be specified for **size** based rotation.

For example, a component names scheduler could use a format specifier as `scheduler_%D_%N.log`. The log file names for this component would be
```
scheduler_20180727_000001.log
scheduler_20180727_000002.log
scheduler_20180727_000003.log
scheduler_20180728_000000.log
scheduler_20180728_000001.log
```

### Logging interface

Every file using liblog must include `logmgr.h`. If file logger is needed, `flogger.h` must be included as well.

C++ stream based interface:
```c++
/*
 * 1. class_name has the class name.
 * 2. nullptr can be used for class name.
 * 3. Note an additional parameter for WARNING_STRM
 *    and ERROR_STRM.
 */
TRACE_STRM(class_name)              << "this is a trace severity log message"
                                    << snf::log::record::endl;
DEBUG_STRM(class_name)              << "this is a debug severity log message"
                                    << snf::log::record::endl;
INFO_STRM(nullptr)                  << "this is an info severity log message"
                                    << snf::log::record::endl;
WARNING_STRM(class_name)            << "this is a warning severity log message"
                                    << snf::log::record::endl;
WARNING_STRM(class_name, errno)     << "this is another warning severity log message with errno"
                                    << snf::log::record::endl;
ERROR_STRM(class_name)              << "this is an error severity log message"
                                    << snf::log::record::endl;
ERROR_STRM(nullptr, GetLastError()) << "this is another error severity log message with GetLastError()"
                                    << snf::log::record::endl;
```
`snf::log::record::endl` is the record terminator/finalizer inspired by `std::endl`. It not only finalizes the log record but also pushes it to the registered loggers.

printf format specifier based interface:
```c++
/*
 * 1. class_name has the class name.
 * 2. nullptr can be used for class name.
 * 3. Note LOG_SYSERR takes an additional argument for error.
 */
LOG_TRACE(class_name, "this is %s %s log message", "a", "trace");
LOG_DEBUG(class_name, "this is %s %s log message", "a", "debug");
LOG_INFO(nullptr, "this is %s %s log message", "an", "info");
LOG_WARNING(class_name, "this is %s %s log message", "a", "warning");
LOG_ERROR(class_name, "this is %s %s log message", "an", "error");
LOG_SYSERR(nullptr, errno, "this is %s %s log message with errno", "an", "error");
LOG_SYSERR(class_name, GetLastError(), "this is %s %s log message with GetLastError()", "an", "error");
```

### Log Manager

At the heart of liblog is a logging manager. It is a singleton class. Every logging interface uses log manager to log. It provides the following functionality:
1. Ability to add/delete loggers.
2. Provide a default console logger when no loggers are registered.
3. Parsing log configuration file and creating/registering loggers.
4. Logging a log record.

API | Functionality
--- | -------------
snf::log::manager::instance | Obtains reference to the log manager.
snf::log::manager::add_logger | Adds/registers a logger.
snf::log::manager::remove_logger | Removes/de-register a logger.
snf::log::manager::load | Parses and adds loggers from configuration file.
snf::log::manager::log | Logs the log record. The record is pushed to all the registered loggers.

#### Console Logger

Console logger logs to the console. It has 4 main attributes:
1. **severity** to control log verbosity.
2. **format** to control log format.
3. **destination** to control where the message is logged: *out*, *err*, *var*. If the destination is *var*, **warning** and **error** messages are sent to *stderr* and the remaining ones are logged to *stdout*.
```c++
// Adding console logger
snf::log::console_logger *clog = new snf::log::console_logger(
					snf::log::severity::warning,  // default is all
					"%D %T %p %t [%s] [%f] %m");  // default is "[%s] [%f] %m"
clog->set_destination(snd::log::console_logger::destination::err);    // default is var
snf::log::manager::instance().add_logger(clog);
```

#### File Logger

File logger logs to files. It is more complex and provides a much richer set of functionality. Its main attributes are:
1. **severity** to control log verbosity.
2. **format** to control log format.
3. **path** to find where to log.
4. **make_path** to create **path** if it does not exist.
5. **name_format** to control log file name format.
6. **sync** to sync log message with data store immediately.
7. **rotation scheme** to control log rollover.
8. **retention scheme** to control how many log files to retain.
```c++
// Adding file logger
snf::log::file_logger *flog = new snf::log::file_logger(
					log_path                      // default is "."
					snf::log::severity::warning,  // default is all
					"%D %T %p %t [%s] [%f] %m");  // default is "%D %T %p.%t [%s] [%F:%c.%f.%l] %m"
flog->make_path(true);                                                // default is false
flog->set_name_format("scheduler_%D_%N.log");                         // default is "%D_%N.log"
flog->sync(true);                                                     // default is flase
// flog->set_rotation(rot);                                           // discussed later
// flog->set_retention(ret);                                          // discussed later
snf::log::manager::instance().add_logger(flog);
```

#### Log Rotation

Log files can be huge in size making it difficult to open, process, and analyze. It is a good idea to rollover to newer files periodically. A few options are provided:
1. No rotation `snf::log::rotation::scheme::none`.
2. Rotate daily at midnight `snf::log::rotation::scheme::daily`. Log name format must have %D in it.
3. Rotate based on log file size `snf::log::rotation::scheme::by_day`. Log name format must have %N in it.
4. A combination of *2* and *3*. Log name format must have %D and %N in it.
```c++
snf::log::rotation *rot_by_day = new snf::log::rotation(
					snf::log::rotation::scheme::daily);   // default is none
snf::log::rotation *rot_by_size = new snf::log::rotation(
					snf::log::rotation::scheme::by_size,
					100000);                              // default is 100MB
snf::log::rotation *rot_by_day_and_size = new snf::log::rotation(
					snf::log::rotation::scheme::daily |
					snf::log::rotation::scheme::by_size,
					100000);
flog->set_rotation(rot_by_day_and_size);
delete rot_by_size;
delete rot_by_day;
// do not delete rot_by_day_and_size. file logger is responsible for deleting it.
```

#### Log retention

Log files can be huge in numbers as well. It is a good idea to retain only the last few files. The options provided for this are:
1. Retain all `snf::log::retention::scheme::all`.
2. Retain files for last N days `snf::log::retention::scheme::last_n_days`.
3. Retain last N files `snf::log::retention::scheme::last_n_files`.

The logs are merged every time a new log file is opened i.e. when the log file is first opened and when the log rotation occurs.
```c++
snf::log::retention ret_n_days = new snf::log::retention(
					snf::log::retention::scheme::log_n_days, // default is all
                                        3);                                      // default is 10 days
snf::log::retention ret_n_files = new snf::log::retention(
					snf::log::retention::scheme::log_n_files, // default is all
                                        20);                                      // default is 10 days
flog->set_retention(ret_n_days);
delete ret_n_files;
// do not delete ret_n_days. file logger is responsible for deleting it.
```

### Log Configuration

Creating logger manually is fine for local testing. But for production environment, it is often convenient to specify configuration via file. liblog supports JSON configuration file. The configuration file is a JSON record with multiple objects, one for each component/module/process:
```jsob
{
    "component_1" : [ list_of_loggers ],
    "component_2" : [ list_of_loggers ]
}
```

Here is an example for one component, by the name *schedulera*, only. There are two loggers registered for *scheduler* : the console logger and the file based logger.
```json
{
    "scheduler" : [
        {
            "type" : "console",
            "severity" : "INFO",
            "format" : "json-pretty",
            "destination" : "stdout"
        },
        {
            "type" : "file",
            "severity" : "TRACE",
            "format" : "json",
            "name_format" : "scheduler_%D_%N.log",
            "path" : ".",
            "make_path" : false,
            "sync" : true,
            "rotation" : {
                    "scheme" : "daily | by_size",
                    "size" : 500000
            },
            "retention" : {
                    "scheme" : "last_n_files",
                    "argument" : 5
            }
        }
    ]
}
```
The scheduler component should load the configuration file as following:
```c++
snf::log::manager::instance().load(conf_file, "scheduler");
```
and proceed with logging. One call is all that is needed to set up the logging subsystem in this case.

### What happens when a process forks?

If a process forks and the child process exec()s immediately, there is nothing to be done. If the process
forks and does not exec(), simply call:
```c++
snf::log::manager::reset()
```
This will reset the cached pid for the child process. It will also ask all the registered loggers to close any open file. The next time a message is logged, the file will be reopened and things should proceed smoothly. Btw, there is no file based locking yet. Multiple processes logging to the same file can cause some log corruption but the chances are remote.
