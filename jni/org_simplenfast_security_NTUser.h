/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_simplenfast_security_NTUser */

#ifndef _Included_org_simplenfast_security_NTUser
#define _Included_org_simplenfast_security_NTUser
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_simplenfast_security_NTUser
 * Method:    login0
 * Signature: (Ljava/lang/String;Ljava/lang/String;[C)J
 */
JNIEXPORT jlong JNICALL Java_org_simplenfast_security_NTUser_login0
  (JNIEnv *, jobject, jstring, jstring, jcharArray);

/*
 * Class:     org_simplenfast_security_NTUser
 * Method:    execute0
 * Signature: (JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;[J)J
 */
JNIEXPORT jlong JNICALL Java_org_simplenfast_security_NTUser_execute0
  (JNIEnv *, jobject, jlong, jstring, jstring, jstring, jlongArray);

/*
 * Class:     org_simplenfast_security_NTUser
 * Method:    getExitCode0
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_org_simplenfast_security_NTUser_getExitCode0
  (JNIEnv *, jobject, jlong, jlong);

/*
 * Class:     org_simplenfast_security_NTUser
 * Method:    logout0
 * Signature: (J)Z
 */
JNIEXPORT jboolean JNICALL Java_org_simplenfast_security_NTUser_logout0
  (JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif
