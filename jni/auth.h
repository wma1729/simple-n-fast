#ifndef _AUTH_H_
#define _AUTH_H_

#include <jni.h>

char *jcharArrayToCString(JNIEnv *, jcharArray);

#if defined(WINDOWS)

HANDLE nt_login(JNIEnv *, const char *, const char *, const char *);
bool   nt_logout(HANDLE);

#else // !WINDOWS

#include <security/pam_appl.h>

bool pam_login(JNIEnv *, const char *, const char *, void *, pam_handle_t **);
bool pam_logout(pam_handle_t *);

#endif

#endif // _AUTH_H_
