#include "ex.h"

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
	jclass exClass = env->FindClass("org/simplenfast/nex/NativeException");
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
