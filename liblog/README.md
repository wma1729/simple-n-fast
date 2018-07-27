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
```
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
