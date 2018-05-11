#include "common.h"
#include <jni.h>
#include <list>
#include "config.h"
#include "dll.h"
#include "filesystem.h"
#include "log.h"
#include "error.h"

struct DaemonArgs {
	std::string             name;           // daemon/service name
	std::string             home;           // daemon home
	std::string             pidPath;        // pid file (useful on UNIXes only)
	std::string             logPath;        // log path
	std::string             jvmLibPath;     // JRE to use
	std::list<std::string>  jvmOptions;     // JVM options
	std::string             startClass;     // start class (Java entry point)
	std::string             startMethod;    // start method in start class
	std::string             stopClass;      // stop class (Jave exit point)
	std::string             stopMethod;     // stop method in stop class
};

extern Logger       *TheLogger;
extern bool         TheVerbosity;

static DaemonArgs   TheDaemonArgs;
static JavaVM       *TheJVM;

typedef jint (JNICALL *create_jvm_t)(JavaVM **, void**, void *);

/*
 * Log the daemon args.
 */
static void
LogDaemonArgs(void)
{
	const char  *caller = "DaemonArgs";
	int         i = 0;

	Log(INF, caller, "name = %s", TheDaemonArgs.name.c_str());
	Log(INF, caller, "home = %s", TheDaemonArgs.home.c_str());
	Log(INF, caller, "pidPath = %s", TheDaemonArgs.pidPath.c_str());
	Log(INF, caller, "logPath = %s", TheDaemonArgs.logPath.c_str());
	Log(INF, caller, "jvmLibPath = %s", TheDaemonArgs.jvmLibPath.c_str());

	std::list<std::string>::const_iterator I;
	for (I = TheDaemonArgs.jvmOptions.begin(); I != TheDaemonArgs.jvmOptions.end(); ++I) {
		Log(INF, caller, "jvmOptions[%d] = %s", i++, I->c_str());
	}

	Log(INF, caller, "startClass = %s", TheDaemonArgs.startClass.c_str());
	Log(INF, caller, "startMethod = %s", TheDaemonArgs.startMethod.c_str());
	Log(INF, caller, "stopClass = %s", TheDaemonArgs.stopClass.c_str());
	Log(INF, caller, "stopMethod = %s", TheDaemonArgs.stopMethod.c_str());
}

/*
 * Calls java method. It is a static method with no args returning void.
 *
 * @param env - the JNI environment.
 * @param className - the java class name
 * @param methodName - the method name.
 * @param signature - the method signature, always ()V.
 *
 * @return JNI_OK on success, non-zero error code on failure.
 */
static jint
CallJavaMethod(JNIEnv *env, const char *className, const char *methodName, const char *signature)
{
	const char  *caller = "CallJavaMethod";
	jint        retval = JNI_OK;

	Log(DBG, caller, "calling %s.%s%s",
			className, methodName, signature);

	jclass klass = env->FindClass(className);
	if (klass == 0) {
		Log(ERR, caller, "failed to find class %s", className);
		retval = JNI_ERR;
	} else {
		jmethodID mid = env->GetStaticMethodID(klass, methodName, signature);
		if (mid == 0) {
			Log(ERR, caller, "failed to find method %s%s",
				methodName, signature);
			retval = JNI_ERR;
		} else {
			env->CallStaticVoidMethod(klass, mid);
		}
	}

	if (env->ExceptionOccurred()) {
		env->ExceptionClear();
		Log(ERR, caller, "exception occurred!");
		if (retval == JNI_OK)
			retval = JNI_ERR;
	}

	if (retval != JNI_OK) {
		Log(ERR, caller, "failed to call %s%s in class %s",
						methodName,
						signature,
						className);
	}

	return retval;
}

/*
 * Loads JVM and call the start method in start class.
 * @return JNI_OK on success, non-zero error code on failure.
 */
static jint
StartDaemon(void)
{
	const char      *caller = "StartDaemon";
	int             i = 0, count;
	jint            retval = JNI_OK;
	JNIEnv          *env;
	JavaVMInitArgs  vmArgs;
	JavaVMOption    *options;
	snf::dll        lib(TheDaemonArgs.jvmLibPath);
	create_jvm_t    pCreateJVM = (create_jvm_t)lib.symbol("JNI_CreateJavaVM");

	if (pCreateJVM == 0) {
		return JNI_ERR;
	}

	Log(DBG, caller, "successfully loaded %s", TheDaemonArgs.jvmLibPath.c_str());

	count = (int) TheDaemonArgs.jvmOptions.size();
	options = (JavaVMOption *)calloc(count, sizeof(JavaVMOption));

	std::list<std::string>::const_iterator I;
	for (I = TheDaemonArgs.jvmOptions.begin();
		 I != TheDaemonArgs.jvmOptions.end();
		 ++I) {
		options[i].optionString = strdup(I->c_str());
		options[i].extraInfo = 0;
		++i;
	}

	vmArgs.version = JNI_VERSION_1_8;
	vmArgs.nOptions = count;
	vmArgs.options = options;
	vmArgs.ignoreUnrecognized = true;

	retval = (pCreateJVM)(&TheJVM, (void**)&env, &vmArgs);

	for (i = 0; i < count; ++i)
		free(options[i].optionString);

	free(options);

	if (retval != JNI_OK) {
		Log(ERR, caller, "failed to create Java VM: %d", retval);
		return retval;
	} else {
		Log(DBG, caller, "successfully created JVM");
	}

	/*
	 * We are getting into the Java code. The current thread will
	 * impersonates itself into the java thread and call the java code.
	 */

	retval = CallJavaMethod(
			env,
			TheDaemonArgs.startClass.c_str(),
			TheDaemonArgs.startMethod.c_str(),
			"()V");

	/*
	 * We are back from the Java code. Now it is time to detach the
	 * the current thread from the JVM.
	 */

	jint r = TheJVM->DetachCurrentThread();
	if (r != JNI_OK) {
		Log(ERR, caller, "failed to detach from the JVM");
		if (retval == JNI_OK)
			retval = r;
	}

	return retval;
}

/*
 * Attach to the loaded JVM and calls stop method in stop class.
 * @return JNI_OK on success, non-zero error code on failure.
 */
static jint
StopDaemon(void)
{
	const char  *caller = "StopDaemon";
	jint        retval = JNI_OK;
	JNIEnv      *env;

	if (TheJVM) {
		retval = TheJVM->AttachCurrentThread((void**)&env, NULL);
		if (retval != JNI_OK) {
			Log(ERR, caller, "failed to attach to the JVM");
		} else {
			Log(DBG, caller, "successfully attached to the JVM");

			/*
			 * We are getting into the Java code. The current thread will
			 * impersonates itself into the java thread and call the java code.
			 */

			retval = CallJavaMethod(
						env,
						TheDaemonArgs.stopClass.c_str(),
						TheDaemonArgs.stopMethod.c_str(),
						"()V");

			/*
			 * We are back from the Java code. Now it is time to detach the
			 * the current thread from the JVM.
			 */

			jint r = TheJVM->DetachCurrentThread();
			if (r != JNI_OK) {
				Log(ERR, caller, "failed to detach from the JVM");
				if (retval == JNI_OK)
					retval = r;
			}
		}
	} else {
		Log(WRN, caller, "JVM not found");
	}

	return retval;
}

#if defined(_WIN32)
	#include "ntservice.cpp"
#else
	#include "unixdaemon.cpp"
#endif

static int
Usage(const char *progName)
{
#if defined(_WIN32)
	fprintf(stderr, "Usage: %s [-start|-stop|-chkconf] -config <service_config> [-verbose]\n",
#else
	fprintf(stderr, "Usage: %s [-start|-stop|-chkconf] -config <daemon_config> [-verbose]\n",
#endif
		progName);

	return 1;
}

int
main(int argc, const char **argv)
{
	int         retval = E_ok;
	bool        stop = false;
	bool        checkConfig = false;
	Config      *config = 0;
	const char  *configName = 0;
	const char  *ptr;
	char        progName[MAXPATHLEN + 1];
	char        buf[MAXPATHLEN + 1];

	snf::basename(progName, MAXPATHLEN, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcmp("-start", argv[i]) == 0) {
			stop = false;
		} else if (strcmp("-stop", argv[i]) == 0) {
			stop = true;
		} else if (strcmp("-chkconf", argv[i]) == 0) {
			checkConfig = true;
		} else if (strcmp("-config", argv[i]) == 0) {
			i++;
			if (argv[i] == 0) {
				fprintf(stderr, "%s\n", "argument to -config missing");
				return 1;
			} else {
				configName = argv[i];
			}
		} else if (strcmp("-verbose", argv[i]) == 0) {
			TheVerbosity = true;
		} else {
			return Usage(progName);
		}
	}

	if (configName) {
		config = DBG_NEW Config(configName);
	} else {
		fprintf(stderr, "%s\n", "-config option not specified");
		return 1;
	}

	TheDaemonArgs.name = config->get("NAME", progName);

	ptr = config->get("HOME");
	if (ptr != 0) {
		TheDaemonArgs.home = ptr;
	} else {
		if (FileSystem::getHome(buf, MAXPATHLEN) != E_ok) {
			fprintf(stderr, "%s\n", "failed to find HOME");
			return 1;
		}
		TheDaemonArgs.home = buf;
	}

	ptr = config->get("PID_PATH");
	if (ptr == 0) {
		ptr = TheDaemonArgs.home.c_str();
	}

	int n = snprintf(buf, MAXPATHLEN, "%s%c.%s.pid",
				ptr,
				PATH_SEP,
				progName);
	buf[n] = '\0';
	TheDaemonArgs.pidPath = buf;

	TheDaemonArgs.logPath = config->get("LOG_PATH", TheDaemonArgs.home);

	ptr = config->get("JVM_LIB_PATH");
	if (ptr == 0) {
		fprintf(stderr, "%s\n", "failed to find JVM_LIB_PATH");
		return 1;
	}

	TheDaemonArgs.jvmLibPath = ptr;

	for (int i = 0; i < 128; ++i) {
		snprintf(buf, MAXPATHLEN, "%s_%d", "JVM_OPTIONS", i);
		ptr = config->get(buf);
		if (ptr != 0) {
			TheDaemonArgs.jvmOptions.push_back(ptr);
		} else {
			break;
		}
	}

	ptr = config->get("START_CLASS");
	if (ptr == 0) {
		fprintf(stderr, "%s\n", "failed to find START_CLASS");
		return 1;
	}

	TheDaemonArgs.startClass = ptr;
	TheDaemonArgs.startMethod = config->get("START_METHOD", "start");

	ptr = config->get("STOP_CLASS");
	if (ptr == 0) {
		fprintf(stderr, "%s\n", "failed to find STOP_CLASS");
		return 1;
	}

	TheDaemonArgs.stopClass = ptr;
	TheDaemonArgs.stopMethod = config->get("STOP_METHOD", "stop");

	if (checkConfig) {
		LogDaemonArgs();
	} else if (stop) {
		retval = DaemonStop();
	} else {
		retval = Daemonize();
	}

	return retval;
}
