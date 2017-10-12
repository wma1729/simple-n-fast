#include <pwd.h>
#include <grp.h>
#include "org_simplenfast_jaas_PAMUser.h"
#include "common.h"
#include "auth.h"
#include "ex.h"

static bool
set_principals(
	JNIEnv *env,
	jobject obj,
	const char *user)
{
	const char      *who = "set_principals";
	int             retval = 0;
	struct passwd   *result = 0;
	struct passwd   pwd;
	int             ngrps = 0;
	jlong           *jgrps = 0;
	char            message[512];
	char            errbuf[ERRSTRLEN + 1];
	jclass          cls;
	jmethodID       mid;

	size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);	
	if (bufsize < 0) {
		bufsize = 16 * 1024;
	}

	char *buf = (char *)malloc(bufsize);
	if (buf == 0) {
		ThrowNativeException(
			env,
			"malloc() of passwd buffer failed",
			errno,
			GetErrorStr(errbuf, ERRSTRLEN, errno),
			who,
			__FILE__,
			__LINE__);

		return false;
	}

	errno = 0;
	retval = getpwnam_r(user, &pwd, buf, bufsize, &result);
	if ((retval != 0) || (result == 0)) {
		free(buf);

		if (retval == 0) {
			snprintf(message, sizeof(message),
				"getpwnam_r() for user %s did not find any data", user);
		} else {
			snprintf(message, sizeof(message),
				"getpwnam_r() for user %s failed", user);
		}

		ThrowNativeException(
			env,
			message,
			retval,
			GetErrorStr(errbuf, ERRSTRLEN, retval),
			who,
			__FILE__,
			__LINE__);

		return false;
	}

	getgrouplist(pwd.pw_name, pwd.pw_gid, 0, &ngrps);
	if (ngrps > 0) {
		gid_t *grps = (gid_t *)calloc(ngrps, sizeof(gid_t));
		if (grps == 0) {
			free(buf);

			ThrowNativeException(
				env,
				"malloc() of group ID buffer failed",
				errno,
				GetErrorStr(errbuf, ERRSTRLEN, errno),
				who,
				__FILE__,
				__LINE__);

			return false;
		}

		getgrouplist(pwd.pw_name, pwd.pw_gid, grps, &ngrps);

		jgrps = (jlong *)calloc(ngrps, sizeof(jlong));
		if (jgrps == 0) {
			free(buf);
			free(grps);

			ThrowNativeException(
				env,
				"malloc() of Java group ID buffer failed",
				errno,
				GetErrorStr(errbuf, ERRSTRLEN, errno),
				who,
				__FILE__,
				__LINE__);

			return false;
		}

		for (int i = 0; i < ngrps; ++i) {
			jgrps[i] = (jlong)(grps[i]);
		}

		free(grps);
	}

	cls = env->GetObjectClass(obj);

	mid = env->GetMethodID(
			cls,
			"setUID",
			"(J)V");
	env->CallVoidMethod(obj, mid, long(pwd.pw_uid));

	mid = env->GetMethodID(
			cls,
			"setGID",
			"(J)V");
	env->CallVoidMethod(obj, mid, long(pwd.pw_gid));

	free(buf);

	if (ngrps > 0) {
		jlongArray jgids = env->NewLongArray(ngrps);
		env->SetLongArrayRegion(jgids, 0, ngrps, jgrps);
		free(jgrps);

		mid = env->GetMethodID(
				cls,
				"setSupplementaryGIDs",
				"([J)V");
		env->CallVoidMethod(obj, mid, jgids);
		env->DeleteLocalRef(jgids);
	}

	return true;
}

JNIEXPORT jlong JNICALL
Java_org_simplenfast_jaas_PAMUser_login0(
	JNIEnv *env,
	jobject obj,
	jstring svc,
	jstring usr,
	jcharArray pwd)
{
	pam_handle_t    *pamh;
	const char      *service = env->GetStringUTFChars(svc, NULL);
	const char      *user = env->GetStringUTFChars(usr, NULL);
	char            *password = jcharArrayToCString(env, pwd);

	if (password == 0) {
		return 0;
	}

	if (pam_login(env, service, user, password, &pamh)) {
		if (!set_principals(env, obj, user)) {
			pam_logout(pamh);
			pamh = 0;
		}
	} else {
		pamh = 0;
	}

	free(password);
	env->ReleaseStringUTFChars(usr, user);
	env->ReleaseStringUTFChars(svc, service);

	return (long)pamh;
}

JNIEXPORT jboolean JNICALL
Java_org_simplenfast_jaas_PAMUser_logout0(
	JNIEnv *env,
	jobject obj,
	jlong ctx)
{
	pam_handle_t *pamh = (pam_handle_t *)ctx;
	if (pam_logout(pamh)) {
		return JNI_TRUE;
	}

	return JNI_FALSE;
}
