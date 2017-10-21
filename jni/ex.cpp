#include "ex.h"

/**
 * Throws NativeException in the Java code. The idea is to provide
 * the cause of the failure in native code as clearly as possible.
 * The native code, at least the one I have, logs nothing. The only
 * way to debug is through the exception.
 *
 * @param [in] env         - JNI environment
 * @param [in] message     - message describing the exception
 * @param [in] errorCode   - native error code (often OS error code)
 * @param [in] errorString - native error string corresponding to
 *                           the error code
 * @param [in] methodName  - method name where the error occurred
 * @param [in] fileName    - file name where the error occurred
 * @param [in] lineNumber  - line number where the error occurred
 */
void
ThrowNativeException(
	JNIEnv *env,
	const char *message,
	int errorCode,
	const char *errorString,
	const char *methodName,
	const char *fileName,
	int lineNumber)
{
	jclass exClass = env->FindClass("org/simplenfast/security/NativeException");
	if (exClass == 0) {
		return;
	}

	jobject exObject;

	if (errorCode == -1) {
		jmethodID exConstructor = env->GetMethodID(
				exClass,
				"<init>",
				"(Ljava/lang/String;)V");
		exObject = env->NewObject(
				exClass,
				exConstructor,
				env->NewStringUTF(message));
	} else {
		jmethodID exConstructor = env->GetMethodID(
				exClass,
				"<init>",
				"(Ljava/lang/String;ILjava/lang/String;)V");
		exObject = env->NewObject(
				exClass,
				exConstructor,
				env->NewStringUTF(message),
				errorCode,
				env->NewStringUTF(errorString));
	}

	/*
	 * Add the details to the java exception stack trace.
	 */
	jmethodID exMethod = env->GetMethodID(
				exClass,
				"addToStackTrace",
				"(Ljava/lang/String;Ljava/lang/String;I)V");
	if (exMethod != 0) {
		env->CallVoidMethod(
				exObject,
				exMethod,
				env->NewStringUTF(methodName),
				env->NewStringUTF(fileName),
				lineNumber);
	}

	env->Throw((jthrowable)exObject);

	env->DeleteLocalRef(exClass);

	return;
}
