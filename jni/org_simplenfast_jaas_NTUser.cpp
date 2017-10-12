#include <Windows.h>
#include <Sddl.h>
#include "org_simplenfast_jaas_NTUser.h"
#include "common.h"
#include "auth.h"
#include "ex.h"

static void *
get_token_info(
	JNIEnv *env,
	HANDLE hToken,
	TOKEN_INFORMATION_CLASS tClass)
{
	const char  *who = "get_token_info";
	void        *tInfo = 0;
	DWORD       error = 0;
	DWORD       tInfoLen = 0;
	DWORD       retLen = 0;
	char        errbuf[ERRSTRLEN + 1];

	if (!GetTokenInformation(hToken, tClass, 0, 0, &retLen)) {
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			tInfoLen = retLen;
			tInfo = malloc(tInfoLen);
			if (tInfo == 0) {
				ThrowNativeException(
					env,
					"malloc() of token information buffer failed",
					errno,
					GetErrorStr(errbuf, ERRSTRLEN, errno),
					who,
					__FILE__,
					__LINE__);
			}
		} else {
			error = GetLastError();

			ThrowNativeException(
				env,
				"GetTokenInformation() to fetch token information length failed",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
		}
	}

	if (tInfo && !GetTokenInformation(hToken, tClass, tInfo, tInfoLen, &retLen)) {
		error = GetLastError();

		free(tInfo);
		tInfo = 0;

		ThrowNativeException(
			env,
			"GetTokenInformation() to fetch token information failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);
	}

	return tInfo;
}

static jstring
get_sid_string(
	JNIEnv *env,
	PSID sid)
{
	const char  *who = "get_sid_string";
	char        errbuf[ERRSTRLEN + 1];

	if (IsValidSid(sid)) {
		char    *sidStr = 0;

		if (ConvertSidToStringSid(sid, &sidStr)) {
			jstring jstr = env->NewStringUTF(sidStr);
			LocalFree(sidStr);
			return jstr;
		} else {
			int error = GetLastError();

			ThrowNativeException(
				env,
				"ConvertSidToStringSid() failed",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
		}
	} else {
		ThrowNativeException(
			env,
			"IsValidSid() failed",
			-1,
			0,
			who,
			__FILE__,
			__LINE__);
	}

	return 0;
}

static jstring
get_domain_sid(
	JNIEnv *env,
	const char *domain)
{
	const char      *who = "get_domain_sid";
	int             error = 0;
	PSID            sid = 0;
	DWORD           sidLen = 0;
	char            *refDomain = 0;
	DWORD           refDomainLen = 0;
	SID_NAME_USE    sidUse;
	jstring         jstr = 0;
	char            message[512];
	char            errbuf[ERRSTRLEN + 1];

	if (!LookupAccountName(
			0,
			domain,
			sid,
			&sidLen,
			refDomain,
			&refDomainLen,
			&sidUse)) {
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			sid = (PSID)malloc(sidLen);
			refDomain = (char *)malloc(refDomainLen);

			if ((sid == 0) || (refDomain == 0)) {
				snprintf(message, sizeof(message),
					"malloc() of %s buffer failed",
					(sid == 0) ? "SID" : "reference domain");

				ThrowNativeException(
					env,
					message,
					errno,
					GetErrorStr(errbuf, ERRSTRLEN, errno),
					who,
					__FILE__,
					__LINE__);

				if (sid) free(sid);
				if (refDomain) free(refDomain);
			}
		} else {
			error = GetLastError();

			snprintf(message, sizeof(message),
				"GetTokenInformation() to fetch domain (%s) SID failed",
				domain);

			ThrowNativeException(
				env,
				message,
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
		}
	}

	if (sid != 0) {
		if (!LookupAccountName(
				0,
				domain,
				sid,
				&sidLen,
				refDomain,
				&refDomainLen,
				&sidUse)) {

			error = GetLastError();

			snprintf(message, sizeof(message),
				"GetTokenInformation() to fetch domain (%s) SID failed",
				domain);

			ThrowNativeException(
				env,
				message,
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
		} else {
			jstr = get_sid_string(env, sid);
		}

		free(refDomain);
		free(sid);
	}

	return jstr;
}

static bool
set_principals(
	JNIEnv *env,
	jobject obj,
	HANDLE hToken,
	const char *domain)
{
	TOKEN_USER          *tUser = 0;
	TOKEN_PRIMARY_GROUP *tPrimGrp = 0;
	TOKEN_GROUPS        *tGroups = 0;
	jstring             userSid = 0;
	jstring             domainSid = 0;
	jstring             primGrpSid = 0;
	jobjectArray        groupSids = 0;
	bool                error = false;
	jclass              cls;
	jmethodID           mid;

	do {
		tUser = (TOKEN_USER *)get_token_info(env, hToken, TokenUser);
		if (tUser == 0) {
			error = true;
			break;
		}

		tPrimGrp = (TOKEN_PRIMARY_GROUP *)get_token_info(env, hToken, TokenPrimaryGroup);
		if (tPrimGrp == 0) {
			error = true;
			break;
		}

		tGroups = (TOKEN_GROUPS *)get_token_info(env, hToken, TokenGroups);
		if (tGroups == 0) {
			error = true;
			break;
		}

		userSid = get_sid_string(env, tUser->User.Sid);
		if (userSid == 0) {
			error = true;
			break;
		}

		if (domain && (*domain != '.')) {
			domainSid = get_domain_sid(env, domain);
			if (domainSid == 0) {
				error = true;
				break;
			}
		}

		primGrpSid = get_sid_string(env, tPrimGrp->PrimaryGroup);
		if (primGrpSid == 0) {
			error = true;
			break;
		}

		if (tGroups->GroupCount > 0) {
			jclass jcls = env->FindClass("java/lang/String");
			groupSids = env->NewObjectArray(tGroups->GroupCount, jcls, 0);
			int jidx = 0;

			for (DWORD i = 0; i < tGroups->GroupCount; ++i) {
				jstring jstr = get_sid_string(env, tGroups->Groups[i].Sid);
				if (jstr == 0) {
					error = true;
					break;
				}
				env->SetObjectArrayElement(groupSids, jidx++, jstr);
			}

			if (error) {
				for (int i = 0; i < jidx; ++i) {
					jobject jobj = env->GetObjectArrayElement(groupSids, i);
					env->DeleteLocalRef(jobj);
				}
			}
		}
	} while (0);

	if (tGroups) free(tGroups);
	if (tPrimGrp) free (tPrimGrp);
	if (tUser) free(tUser);

	if (error) {
		if (groupSids) env->DeleteLocalRef(groupSids);
		if (primGrpSid) env->DeleteLocalRef(primGrpSid);
		if (domainSid) env->DeleteLocalRef(domainSid);
		if (userSid) env->DeleteLocalRef(userSid);
		return false;
	}

	cls = env->GetObjectClass(obj);

	mid = env->GetMethodID(
			cls,
			"setUserSID",
			"(Ljava/lang/String;)V");
	env->CallVoidMethod(obj, mid, userSid);
	env->DeleteLocalRef(userSid);

	if (domainSid) {
		mid = env->GetMethodID(
				cls,
				"setDomainSID",
				"(Ljava/lang/String;)V");
		env->CallVoidMethod(obj, mid, domainSid);
		env->DeleteLocalRef(domainSid);
	}

	mid = env->GetMethodID(
			cls,
			"setPrimaryGroupSID",
			"(Ljava/lang/String;)V");
	env->CallVoidMethod(obj, mid, primGrpSid);
	env->DeleteLocalRef(primGrpSid);

	mid = env->GetMethodID(
			cls,
			"setGroupSIDs",
			"([Ljava/lang/String;)V");
	env->CallVoidMethod(obj, mid, groupSids);
	env->DeleteLocalRef(groupSids);

	return true;
}

JNIEXPORT jlong JNICALL
Java_org_simplenfast_jaas_NTUser_login0(
	JNIEnv *env,
	jobject obj,
	jstring domain,
	jstring usr,
	jcharArray pwd)
{
	const char  *domainName = env->GetStringUTFChars(domain, NULL);
	const char  *user = env->GetStringUTFChars(usr, NULL);
	char        *password = jcharArrayToCString(env, pwd);
	HANDLE      hToken = NULL;

	if (password == 0) {
		return 0;
	}

	hToken = nt_login(env, domainName, user, password);
	if (hToken != NULL) {
		if (!set_principals(env, obj, hToken, domainName)) {
			nt_logout(hToken);
			hToken = NULL;
		}
	}

	free(password);
	env->ReleaseStringUTFChars(usr, user);
	env->ReleaseStringUTFChars(domain, domainName);

	return HandleToLong(hToken);
}

JNIEXPORT jboolean JNICALL
Java_org_simplenfast_jaas_NTUser_logout0(
	JNIEnv *env,
	jobject obj,
	jlong ctx)
{
	HANDLE hToken = (HANDLE)(ctx);
	if (nt_logout(hToken)) {
		return JNI_TRUE;
	}
	return JNI_FALSE;
}
