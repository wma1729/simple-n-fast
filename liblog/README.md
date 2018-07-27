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
```
{ "class" : "no-class", "error" : 0, "file" : "logtester.cpp", "function" : "main", "lineno" : 66, "pid" : 195, "severity" : "ERR", "text" : "a sample error message", "tid" : 195, "timestamp" : 1532658086959 }
```
or *json-pretty*
```
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
* *%D* Date YYYY/MM/DD
* *%T* Time hh:mm:ss.msec
* *%p* Process ID
* *%t* Thread ID
* *%s* Log severity
* *%c* Class name
* *%F* File name
* *%f* Function name
* *%l* Line number
* *%m* Log message/text

The default format for console logger is `[%s] [%f] %m` and the default format for file logger is
`%D %T %p.%t [%s] [%F:%c.%f.%l] %m`.
