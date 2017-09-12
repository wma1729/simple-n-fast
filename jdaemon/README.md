# Java Daemonizer

Java daemonizer `jdaemonizer` is a simple daemon (service on Windows) that can load your Java
classes using [Java's native invocation API](http://docs.oracle.com/javase/8/docs/technotes/guides/jni/spec/invocation.html).
In short, it daemonizes Java programs. `jdaemonizer` relies on a configuration file to get some
parameters for its working.

### Configuration

* `NAME` The daemon/service name. It is really important on Windows. If not specified, it defaults
  to the program name (argv[0] after stripping the path and extension if any).

* `HOME` The daemon/service home. If not specified, the home directory of the current user is
  chosen as the home. On Unix platforms, the daemon does a `chdir` to this path at start-up.

* `PID_PATH` On Unix platforms, the pid of the daemon process is stored in
  `PID_PATH/.<program name>.pid`.
  This file is used
  * at start-up to find if another instance of the daemon is running
  * to get the running daemon pid to send a signal to it when daemon stop is requested.
  
  If not specified, `HOME` is used as the default `PID_PATH`. On Windows, this parameter is ignored.

* `LOG_PATH` The daemon/service log path and not the Java program's log path. This is helpful in
  troubleshooting. If not specified, `HOME` is used as the default `LOG_PATH`.

* `JVM_LIB_PATH` The JVM library path, generally libjvm.so on Unix platforms and jvm.dll on Windows.
  * On Unix platforms, the typical location is `${JRE_HOME}/lib/<architecture>/server/libjvm.so`
    like
    ```sh
    ${JRE_HOME}/lib/sparc/server/libjvm.so (on SPARC 32-bit)
    ${JRE_HOME}/lib/sparcv9/server/libjvm.so (on SPARC 64-bit)
    ${JRE_HOME}/lib/i386/server/libjvm.so (on x86)
    ${JRE_HOME}/lib/x64/server/libjvm.so (on x64/EM64T)
    ${JRE_HOME}/lib/i386/server/libjvm.so (on x86 Linux)
    ${JRE_HOME}/lib/amd64/server/libjvm.so (on amd64 Linux 64-bit)
    ```
  * On Windows, it is usually `%JRE_HOME%\bin\server\jvm.dll`. You can also find it by running
    the following commands:
    ```cmd
    C:\>REG QUERY "HKLM\Software\JavaSoft\Java Runtime Environment"

    HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment
        CurrentVersion    REG_SZ    1.8
        BrowserJavaVersion    REG_SZ    11.144.2

    HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment\1.8
    HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment\1.8.0_144

    C:\>REG QUERY "HKLM\Software\JavaSoft\Java Runtime Environment\1.8" /V RuntimeLib

        HKEY_LOCAL_MACHINE\Software\JavaSoft\Java Runtime Environment\1.8
        RuntimeLib    REG_SZ    C:\Program Files\Java\jre1.8.0_144\bin\server\jvm.dll
    ```

* `JVM_OPTIONS_N` The Java virtual machine options, like the class path. You can specify multiple
  JVM option using JVM_OPTIONS_**0**, JVM_OPTIONS_**1**, and so on. Just remember there should be
  no gaps in the value of **N**.

* `START_CLASS` & `START_METHOD` The Java class and the method name used as entry point into
  the Java code. The method must be a **static** method with **()V** signature i.e. it accepts
  no arguments and returns nothing. This method is invoked by the native thread (that imporsonates
  itself into the Java thread). As such, it should return soon after starting a pure Java starter
  thread.

* `STOP_CLASS` & `STOP_METHOD` The Java class and the method name used to request the Java code
  stop.  The method must be a **static** method with **()V** signature i.e. it accepts no
  arguments and returns nothing. This method, too, is invoked by the native thread (that
  impersonates itself into the Java thread). It should not do anything other than setting a
  termination flag.

### Java daemon example

It is best to understand the usage of jdaemonizer by running a [test daemon](example/README.md).
Follow the instructions here to run your first test daemon.
