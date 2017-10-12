#if !defined(_EX_H_)
#define _EX_H_

#include <jni.h>

void ThrowNativeException
	(JNIEnv *, const char *, int, const char *, const char *, const char *, int);

#endif // _EX_H_
