#include "common.h"
#include <jni.h>
#include <list>
#include "config.h"
#include "dll.h"
#include "filesystem.h"
#include "logmgr.h"
#include "flogger.h"
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

static bool         Verbosity;
static DaemonArgs   TheDaemonArgs;
static JavaVM       *TheJVM;

typedef jint (JNICALL *create_jvm_t)(JavaVM **, void**, void *);

/*
 * Log the daemon args.
 */
static void
LogDaemonArgs(void)
{
	INFO_STRM(nullptr)
		<< "name = " << TheDaemonArgs.name
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "home = " << TheDaemonArgs.home
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "pidPath = " << TheDaemonArgs.pidPath
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "logPath = " << TheDaemonArgs.logPath
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "jvmLibPath = " << TheDaemonArgs.jvmLibPath
		<< snf::log::record::endl;

	int i = 0;
	std::list<std::string>::const_iterator I;
	for (I = TheDaemonArgs.jvmOptions.begin(); I != TheDaemonArgs.jvmOptions.end(); ++I) {
		INFO_STRM(nullptr)
			<< "jvmOptions[" << i++ << "] = " << *I
			<< snf::log::record::endl;
	}

	INFO_STRM(nullptr)
		<< "startClass = " << TheDaemonArgs.startClass
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "startMethod = " << TheDaemonArgs.startMethod
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "stopClass = " << TheDaemonArgs.stopClass
		<< snf::log::record::endl;
	INFO_STRM(nullptr)
		<< "stopMethod = " << TheDaemonArgs.stopMethod
		<< snf::log::record::endl;
}

/*
 * Add file logger.
 */
static void
AddFileLogger()
{
	snf::log::severity sev = snf::log::severity::info;
	if (Verbosity)
		sev = snf::log::severity::trace;

	snf::log::file_logger *flog = DBG_NEW snf::log::file_logger {
					TheDaemonArgs.logPath,
					sev };
	flog->make_path(true);
	snf::log::manager::instance().add_logger(flog);
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
	jint retval = JNI_OK;

	LOG_DEBUG(nullptr, "calling %s.%s%s",
			className, methodName, signature);

	jclass klass = env->FindClass(className);
	if (klass == 0) {
		LOG_ERROR(nullptr, "failed to find class %s", className);
		retval = JNI_ERR;
	} else {
		jmethodID mid = env->GetStaticMethodID(klass, methodName, signature);
		if (mid == 0) {
			LOG_ERROR(nullptr, "failed to find method %s%s",
				methodName, signature);
			retval = JNI_ERR;
		} else {
			env->CallStaticVoidMethod(klass, mid);
		}
	}

	if (env->ExceptionOccurred()) {
		env->ExceptionClear();
		LOG_ERROR(nullptr, "exception occurred!");
		if (retval == JNI_OK)
			retval = JNI_ERR;
	}

	if (retval != JNI_OK) {
		LOG_ERROR(nullptr, "failed to call %s%s in class %s",
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

	DEBUG_STRM(nullptr)
		<< "successfully loaded " << TheDaemonArgs.jvmLibPath
		<< snf::log::record::endl;

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
		ERROR_STRM(nullptr)
			<< "failed to create JVM: " << retval
			<< snf::log::record::endl;
		return retval;
	} else {
		DEBUG_STRM(nullptr)
			<< "successfully created JVM"
			<< snf::log::record::endl; 
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
		ERROR_STRM(nullptr)
			<< "failed to detach from the JVM"
			<< snf::log::record::endl;
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
	jint    retval = JNI_OK;
	JNIEnv  *env;

	if (TheJVM) {
		retval = TheJVM->AttachCurrentThread((void**)&env, NULL);
		if (retval != JNI_OK) {
			ERROR_STRM(nullptr)
				<< "failed to attach to the JVM"
				<< snf::log::record::endl;
		} else {
			DEBUG_STRM(nullptr)
				<< "successfully attached to the JVM"
				<< snf::log::record::endl;

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
				ERROR_STRM(nullptr)
					<< "failed to detach from the JVM"
					<< snf::log::record::endl;
				if (retval == JNI_OK)
					retval = r;
			}
		}
	} else {
		WARNING_STRM(nullptr)
			<< "JVM not found"
			<< snf::log::record::endl;
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
	std::cerr
		<< "Usage: " << progName << " [-start|-stop|-chkconf] -config"
#if defined(_WIN32)
		<< " <service_config>"
#else
		<< " <daemon_config>"
#endif
		<< " [-verbose]" << std::endl;

	return 1;
}

int
main(int argc, const char **argv)
{
	int         retval = E_ok;
	bool        stop = false;
	bool        checkConfig = false;
	snf::config *config = 0;
	const char  *configName = 0;
	const char  *ptr;
	char        progName[MAXPATHLEN + 1];

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
				std::cerr << "argument to -config missing" << std::endl;
				return 1;
			} else {
				configName = argv[i];
			}
		} else if (strcmp("-verbose", argv[i]) == 0) {
			Verbosity = true;
		} else {
			return Usage(progName);
		}
	}

	if (configName) {
		config = DBG_NEW snf::config(configName);
	} else {
		std::cerr << "-config option not specified" << std::endl;
		return 1;
	}

	TheDaemonArgs.name = config->get("NAME", progName);

	ptr = config->get("HOME");
	if (ptr != 0) {
		TheDaemonArgs.home = ptr;
	} else {
		char buf[MAXPATHLEN + 1];

		if (snf::fs::get_home(buf, MAXPATHLEN) != E_ok) {
			std::cerr << "failed to find HOME" << std::endl;
			return 1;
		}
		TheDaemonArgs.home = buf;
	}

	ptr = config->get("PID_PATH");
	if (ptr == 0) {
		ptr = TheDaemonArgs.home.c_str();
	}

	std::ostringstream oss;
	oss << ptr << snf::pathsep() << '.' << progName << ".pid";

	TheDaemonArgs.pidPath = oss.str();

	TheDaemonArgs.logPath = config->get("LOG_PATH", TheDaemonArgs.home);

	ptr = config->get("JVM_LIB_PATH");
	if (ptr == 0) {
		std::cerr << "failed to find JVM_LIB_PATH" << std::endl;
		return 1;
	}

	TheDaemonArgs.jvmLibPath = ptr;

	for (int i = 0; i < 128; ++i) {
		oss.str("");
		oss << "JVM_OPTIONS" << '_' << i;
		ptr = config->get(oss.str());
		if (ptr != 0) {
			TheDaemonArgs.jvmOptions.push_back(ptr);
		} else {
			break;
		}
	}

	ptr = config->get("START_CLASS");
	if (ptr == 0) {
		std::cerr << "failed to find START_CLASS" << std::endl;
		return 1;
	}

	TheDaemonArgs.startClass = ptr;
	TheDaemonArgs.startMethod = config->get("START_METHOD", "start");

	ptr = config->get("STOP_CLASS");
	if (ptr == 0) {
		std::cerr << "failed to find STOP_CLASS" << std::endl;
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
