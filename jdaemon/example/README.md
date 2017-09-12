# jdaemonizer example.

Follow these instructions to run your first test daemon.

### Linux

1. Understand the directory structure. Take some time to understand the Java code.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example> tree
.
├── build.gradle <== Gradle build file
├── README.md    <== This file
├── src          <== Java source code
│   └── main
│       └── java
│           └── org
│               └── simplenfast
│                   └── testdaemon
│                       ├── DaemonNativeBridge.java
│                       └── Worker.java
└── testproduct           <== A simple test product layout
    ├── bin                  <== Binary files go here
    ├── etc
    │   ├── config.unix      <== Sample config file for Unix
    │   └── config.win       <== Sample config file for Windows
    ├── lib                  <== Libraries
    ├── log                  <== Log directory
    └── var                  <== Runtime files

12 directories, 6 files
```

2. Run `gradle build install`. 
```sh
moji@bharat:~/simple-n-fast/jdaemon/example> gradle build install
:compileJava
:processResources UP-TO-DATE
:classes
:jar
:assemble
:compileTestJava UP-TO-DATE
:processTestResources UP-TO-DATE
:testClasses UP-TO-DATE
:test UP-TO-DATE
:check UP-TO-DATE
:build
:install

BUILD SUCCESSFUL

Total time: 5.61 secs
```

3. Now the java files are compiled and the test daemon jar file is stored in `testproduct\lib`.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example> cd testproduct
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct> tree
.
├── bin
├── etc
│   ├── config.unix
│   └── config.win
├── lib
│   └── testdaemon-1.0.jar <== This is the new jar file.
├── log
└── var

5 directories, 3 files
```

4. Copy the jdaemonizer executable to bin directory with a different name.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/bin> cp ~/simple-n-fast/jdaemon/Linux_x64/jdaemonizer TestDaemon
```

5. Take a look at `config.unix` and adjust it appropriately for your envioronment. My `config.unix` looks like this:
```
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/etc> cat config.unix
NAME = TestDaemon
HOME = /home/moji/simple-n-fast/jdaemon/example/testproduct
LOG_PATH = /home/moji/simple-n-fast/jdaemon/example/testproduct/log
PID_PATH= /home/moji/simple-n-fast/jdaemon/example/testproduct/var
JVM_LIB_PATH = /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
# Setting the class path
JVM_OPTIONS_0 = -Djava.class.path=/home/moji/simple-n-fast/jdaemon/example/testproduct/lib/testdaemon-1.0.jar
# Passing daemon.logPath to Java code for logging.
JVM_OPTIONS_1 = -Ddaemon.logPath=/home/moji/simple-n-fast/jdaemon/example/testproduct/log
START_CLASS = org/simplenfast/testdaemon/DaemonNativeBridge
START_METHOD = start
STOP_CLASS = org/simplenfast/testdaemon/DaemonNativeBridge
STOP_METHOD = stop
```

6. It is usually a good idea to make sure that the daemonizer is able to read configuration
correctly.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/bin> TestDaemon -chkconf -config ~/simple-n-fast/jdaemon/example/testproduct/etc/config.unix
[INF] [DaemonArgs] name = TestDaemon
[INF] [DaemonArgs] home = /home/moji/simple-n-fast/jdaemon/example/testproduct
[INF] [DaemonArgs] pidPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/var/.TestDaemon.pid
[INF] [DaemonArgs] logPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/log
[INF] [DaemonArgs] jvmLibPath = /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
[INF] [DaemonArgs] jvmOptions[0] = -Djava.class.path=/home/moji/simple-n-fast/jdaemon/example/testproduct/lib/testdaemon-1.0.jar
[INF] [DaemonArgs] jvmOptions[1] = -Ddaemon.logPath=/home/moji/simple-n-fast/jdaemon/example/testproduct/log
[INF] [DaemonArgs] startClass = org/simplenfast/testdaemon/DaemonNativeBridge
[INF] [DaemonArgs] startMethod = start
[INF] [DaemonArgs] stopClass = org/simplenfast/testdaemon/DaemonNativeBridge
[INF] [DaemonArgs] stopMethod = stop
```

7. Once you are satisfied,  it is time to start the daemon.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/bin> TestDaemon -start -config ~/simple-n-fast/jdaemon/example/testproduct/etc/config.unix -verbose
```

8. Take a look at the pid file.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/var> ls -a
.  ..  .TestDaemon.pid
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/var> cat .TestDaemon.pid
2534moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/var> ps -ef | grep TestDaemon
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/var> ps -ef | grep TestDaemon | grep -v grepmoji      2534     1  0 09:46 ?        00:00:00 TestDaemon -start -config /home/moji/simple-n-fast/jdaemon/example/testproduct/etc/config.unix -verbose
```

9. Time to check the log files. The native code logs to `YYYYMMDD.log` and the Java code logs
   to `daemon.log`.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> ls
20170912.log  daemon.log  daemon.log.lck
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> cat 20170912.log
2017/09/12 09:46:37.969 [2534.2534] [INF] [Daemonize] starting daemon
2017/09/12 09:46:38.002 [2534.2534] [INF] [DaemonArgs] name = TestDaemon
2017/09/12 09:46:38.035 [2534.2534] [INF] [DaemonArgs] home = /home/moji/simple-n-fast/jdaemon/example/testproduct
2017/09/12 09:46:38.068 [2534.2534] [INF] [DaemonArgs] pidPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/var/.TestDaemon.pid
2017/09/12 09:46:38.102 [2534.2534] [INF] [DaemonArgs] logPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/log
2017/09/12 09:46:38.135 [2534.2534] [INF] [DaemonArgs] jvmLibPath = /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
2017/09/12 09:46:38.168 [2534.2534] [INF] [DaemonArgs] jvmOptions[0] = -Djava.class.path=/home/moji/simple-n-fast/jdaemon/example/testproduct/lib/testdaemon-1.0.jar
2017/09/12 09:46:38.202 [2534.2534] [INF] [DaemonArgs] jvmOptions[1] = -Ddaemon.logPath=/home/moji/simple-n-fast/jdaemon/example/testproduct/log
2017/09/12 09:46:38.235 [2534.2534] [INF] [DaemonArgs] startClass = org/simplenfast/testdaemon/DaemonNativeBridge
2017/09/12 09:46:38.268 [2534.2534] [INF] [DaemonArgs] startMethod = start
2017/09/12 09:46:38.323 [2534.2534] [INF] [DaemonArgs] stopClass = org/simplenfast/testdaemon/DaemonNativeBridge
2017/09/12 09:46:38.357 [2534.2534] [INF] [DaemonArgs] stopMethod = stop
2017/09/12 09:46:38.393 [2534.2534] [DBG] [StartDaemon] successfully loaded /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
2017/09/12 09:46:38.502 [2534.2534] [DBG] [StartDaemon] successfully created JVM
2017/09/12 09:46:38.624 [2534.2534] [DBG] [CallJavaMethod] calling org/simplenfast/testdaemon/DaemonNativeBridge.start()V
2017/09/12 09:46:38.752 [2534.2534] [INF] [Daemonize] daemon running
@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> cat daemon.log
Sep 12, 2017 9:46:38 AM org.simplenfast.testdaemon.DaemonNativeBridge start
INFO: start daemon invoked
Sep 12, 2017 9:46:38 AM org.simplenfast.testdaemon.DaemonNativeBridge start
INFO: worker thread started
```

10. The daemon is up and running.

11. Stop the daemon.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/bin> TestDaemon -stop -config ~/simple-n-fast/jdaemon/example/testproduct/etc/config.unix -verbose
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/bin> ps -ef | grep TestDaemon | grep -v grep
```

12. The daemon is stopped. Take a look at the logs now.
```sh
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> ls
20170912.log  daemon.log
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> cat 20170912.log
2017/09/12 09:46:37.969 [2534.2534] [INF] [Daemonize] starting daemon
2017/09/12 09:46:38.002 [2534.2534] [INF] [DaemonArgs] name = TestDaemon
2017/09/12 09:46:38.035 [2534.2534] [INF] [DaemonArgs] home = /home/moji/simple-n-fast/jdaemon/example/testproduct
2017/09/12 09:46:38.068 [2534.2534] [INF] [DaemonArgs] pidPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/var/.TestDaemon.pid
2017/09/12 09:46:38.102 [2534.2534] [INF] [DaemonArgs] logPath = /home/moji/simple-n-fast/jdaemon/example/testproduct/log
2017/09/12 09:46:38.135 [2534.2534] [INF] [DaemonArgs] jvmLibPath = /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
2017/09/12 09:46:38.168 [2534.2534] [INF] [DaemonArgs] jvmOptions[0] = -Djava.class.path=/home/moji/simple-n-fast/jdaemon/example/testproduct/lib/testdaemon-1.0.jar
2017/09/12 09:46:38.202 [2534.2534] [INF] [DaemonArgs] jvmOptions[1] = -Ddaemon.logPath=/home/moji/simple-n-fast/jdaemon/example/testproduct/log
2017/09/12 09:46:38.235 [2534.2534] [INF] [DaemonArgs] startClass = org/simplenfast/testdaemon/DaemonNativeBridge
2017/09/12 09:46:38.268 [2534.2534] [INF] [DaemonArgs] startMethod = start
2017/09/12 09:46:38.323 [2534.2534] [INF] [DaemonArgs] stopClass = org/simplenfast/testdaemon/DaemonNativeBridge
2017/09/12 09:46:38.357 [2534.2534] [INF] [DaemonArgs] stopMethod = stop
2017/09/12 09:46:38.393 [2534.2534] [DBG] [StartDaemon] successfully loaded /usr/lib/jvm/java-8-oracle/jre/lib/amd64/server/libjvm.so
2017/09/12 09:46:38.502 [2534.2534] [DBG] [StartDaemon] successfully created JVM
2017/09/12 09:46:38.624 [2534.2534] [DBG] [CallJavaMethod] calling org/simplenfast/testdaemon/DaemonNativeBridge.start()V
2017/09/12 09:46:38.752 [2534.2534] [INF] [Daemonize] daemon running
2017/09/12 09:58:14.137 [2534.2535] [DBG] [StopDaemon] successfully attached to the JVM
2017/09/12 09:58:14.176 [2534.2535] [DBG] [CallJavaMethod] calling org/simplenfast/testdaemon/DaemonNativeBridge.stop()V
2017/09/12 09:58:14.222 [2534.2534] [DBG] [Daemonize] JVM destroyed
2017/09/12 09:58:14.254 [2534.2534] [INF] [Daemonize] daemon stopped
moji@bharat:~/simple-n-fast/jdaemon/example/testproduct/log> cat daemon.log
Sep 12, 2017 9:46:38 AM org.simplenfast.testdaemon.DaemonNativeBridge start
INFO: start daemon invoked
Sep 12, 2017 9:46:38 AM org.simplenfast.testdaemon.DaemonNativeBridge start
INFO: worker thread started
Sep 12, 2017 9:58:14 AM org.simplenfast.testdaemon.DaemonNativeBridge stop
INFO: stop daemon invoked
Sep 12, 2017 9:58:14 AM org.simplenfast.testdaemon.DaemonNativeBridge stop
INFO: worker thread interrupted
Sep 12, 2017 9:58:14 AM org.simplenfast.testdaemon.Worker run
INFO: termination requested
```

### Windows

Windows is pretty much the same. The differences are:
- The service must be registered with Service Control Manager.
- It is best to start and stop the service using Sevice Controller.
- The account under which the service runs must have 'Logon As Service' right.
  You can use use `accountrights.exe` to achieve this. `accountrights.exe` is
  located in the same folder as `jdaemonizer.exe`.

1. Copy the jdaemonizer executable to bin directory with a different name.
```cmd
Z:\simple-n-fast\jdaemon\example\testproduct\bin>copy C:\simple-n-fast\jdaemon\Windows_x64\jdaemonizer.exe TestDaemon.exe
        1 file(s) copied.
```

2. Take a look at `config.win` and adjust it appropriately for your envioronment. It is recommended to set the NAME in config file same as the executable name. My `config.win` looks like this:
```cmd
C:\simple-n-fast\jdaemon\example\testproduct\etc>type config.win
NAME = TestDaemon
HOME = C:\simple-n-fast\jdaemon\example\testproduct
LOG_PATH = C:\simple-n-fast\jdaemon\example\testproduct\log
PID_PATH= C:\simple-n-fast\jdaemon\example\testproduct\var
# Setting the class path
JVM_OPTIONS_0=-Djava.class.path=C:\simple-n-fast\jdaemon\example\testproduct\lib\testdaemon-1.0.jar
# Passing daemon.logPath to Java code for logging.
JVM_OPTIONS_1=-Ddaemon.logPath=C:\simple-n-fast\jdaemon\example\testproduct\log
JVM_LIB_PATH = C:\Program Files\Java\jre1.8.0_144\bin\server\jvm.dll
START_CLASS = org/simplenfast/testdaemon/DaemonNativeBridge
START_METHOD = start
STOP_CLASS = org/simplenfast/testdaemon/DaemonNativeBridge
STOP_METHOD = stop
```

3. It is usually a good idea to make sure that the daemonizer is able to read configuration
correctly.
```cmd
C:\simple-n-fast\jdaemon\example\testproduct\bin>TestDaemon -chkconf -config C:\simple-n-fast\jdaemon\example\testproduct\etc\config.win
[INF] [DaemonArgs] name = TestDaemon
[INF] [DaemonArgs] home = C:\simple-n-fast\jdaemon\example\testproduct
[INF] [DaemonArgs] pidPath = C:\simple-n-fast\jdaemon\example\testproduct\var\.TestDaemon.pid
[INF] [DaemonArgs] logPath = C:\simple-n-fast\jdaemon\example\testproduct\log
[INF] [DaemonArgs] jvmLibPath = C:\Program Files\Java\jre1.8.0_144\bin\server\jvm.dll
[INF] [DaemonArgs] jvmOptions[0] = -Djava.class.path=C:\simple-n-fast\jdaemon\example\testproduct\lib\testdaemon-1.0.jar
[INF] [DaemonArgs] jvmOptions[1] = -Ddaemon.logPath=C:\simple-n-fast\jdaemon\example\testproduct\log
[INF] [DaemonArgs] startClass = org/simplenfast/testdaemon/DaemonNativeBridge
[INF] [DaemonArgs] startMethod = start
[INF] [DaemonArgs] stopClass = org/simplenfast/testdaemon/DaemonNativeBridge
[INF] [DaemonArgs] stopMethod = stop
```

4. Register the service.
```cmd
C:\simple-n-fast\jdaemon\example\testproduct\bin>sc create TestDaemon binPath= "C:\simple-n-fast\jdaemon\example\testproduct\bin\TestDaemon.exe -config C:\simple-n-fast\jdaemon\example\testproduct\etc\config.win -verbose" DisplayName= "A test service to understand jdaemonizer"
[SC] CreateService SUCCESS

C:\simple-n-fast\jdaemon\example\testproduct\bin>sc description TestDaemon "A test daemon that is always sleeping"
[SC] ChangeServiceConfig2 SUCCESS
```

5. Once the service is registered successfully, start it.
```cmd
C:\simple-n-fast\jdaemon\example\testproduct\bin>sc start TestDaemon

SERVICE_NAME: TestDaemon
        TYPE               : 10  WIN32_OWN_PROCESS
        STATE              : 2  START_PENDING
                                (NOT_STOPPABLE, NOT_PAUSABLE, IGNORES_SHUTDOWN)
        WIN32_EXIT_CODE    : 0  (0x0)
        SERVICE_EXIT_CODE  : 0  (0x0)
        CHECKPOINT         : 0x0
        WAIT_HINT          : 0x7d0
        PID                : 8816
        FLAGS              :
```

6. Stop the service.
```cmd
C:\simple-n-fast\jdaemon\example\testproduct\bin>sc stop TestDaemon

SERVICE_NAME: TestDaemon
        TYPE               : 10  WIN32_OWN_PROCESS
        STATE              : 3  STOP_PENDING
                                (STOPPABLE, NOT_PAUSABLE, ACCEPTS_SHUTDOWN)
        WIN32_EXIT_CODE    : 0  (0x0)
        SERVICE_EXIT_CODE  : 0  (0x0)
        CHECKPOINT         : 0x0
        WAIT_HINT          : 0x0
```

7. You can use the service control manager to manage the service.
