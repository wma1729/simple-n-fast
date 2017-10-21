#include <Windows.h>
#include <Sddl.h>
#include <Userenv.h>
#include "org_simplenfast_security_NTUser.h"
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
	DWORD       tInfoLen = 0;
	DWORD       error = 0;
	DWORD       retLen = 0;
	char        errbuf[ERRSTRLEN + 1];

	if (!GetTokenInformation(hToken, tClass, 0, 0, &retLen)) {
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			tInfoLen = retLen;
			tInfo = malloc(tInfoLen);
			if (tInfo == 0) {
				ThrowNativeException(
					env,
					"malloc of token information buffer failed",
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
				"GetTokenInformation to fetch token information length failed",
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
			"GetTokenInformation to fetch token information failed",
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
				"ConvertSidToStringSid failed",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
		}
	} else {
		ThrowNativeException(
			env,
			"IsValidSid failed",
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
					"malloc of %s buffer failed",
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
				"GetTokenInformation to fetch domain (%s) SID failed",
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
				"GetTokenInformation to fetch domain (%s) SID failed",
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
Java_org_simplenfast_security_NTUser_login0(
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

static bool
CreatePipe(
	JNIEnv *env,
	HANDLE fd[],
	bool readInheritable,
	bool writeInheritable)
{
	const char  *who = "CreatePipe";
	char        errbuf[ERRSTRLEN + 1];
	HANDLE      hRead;
	HANDLE      hWrite;
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };

	if (!::CreatePipe(&hRead, &hWrite, &sa, 0)) {
		int error = GetLastError();

		ThrowNativeException(
			env,
			"CreatePipe failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);

		return false;
	}

	if (readInheritable) {
		if (!SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
			int error = GetLastError();

			CloseHandle(hRead);
			CloseHandle(hWrite);

			ThrowNativeException(
				env,
				"SetHandleInformation failed to make read handle inheritable",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);

			return false;
		}
	}

	if (writeInheritable) {
		if (!SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
			int error = GetLastError();

			CloseHandle(hRead);
			CloseHandle(hWrite);

			ThrowNativeException(
				env,
				"SetHandleInformation failed to make write handle inheritable",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);

			return false;
		}
	}

	fd[0] = hRead;
	fd[1] = hWrite;

	return true;
}

JNIEXPORT jlong JNICALL
Java_org_simplenfast_security_NTUser_execute0(
	JNIEnv *env,
	jobject obj,
	jlong ctx,
	jstring binary,
	jstring commandLine,
	jstring directory,
	jlongArray stdFds)
{
	const char  *who = "Java_org_simplenfast_security_NTUser_execute0";
	DWORD       error = 0;
	HANDLE      hToken = (HANDLE)(ctx);
	HANDLE      hIn[2];
	HANDLE      hOut[2];
	HANDLE      hErr[2];
	void        *envBlk = 0;
	char        errbuf[ERRSTRLEN + 1];

	if (!CreatePipe(env, hIn, true, false)) {
		return (jlong)(0L);
	}

	if (!CreatePipe(env, hOut, false, true)) {
		CloseHandle(hIn[0]);
		CloseHandle(hIn[1]);
		return (jlong)(0L);
	}

	if (!CreatePipe(env, hErr, false, true)) {
		CloseHandle(hIn[0]);
		CloseHandle(hIn[1]);
		CloseHandle(hOut[0]);
		CloseHandle(hOut[1]);
		return (jlong)(0L);
	}

	if (!CreateEnvironmentBlock(&envBlk, hToken, FALSE)) {
		error = GetLastError();

		CloseHandle(hIn[0]);
		CloseHandle(hIn[1]);
		CloseHandle(hOut[0]);
		CloseHandle(hOut[1]);
		CloseHandle(hErr[0]);
		CloseHandle(hErr[1]);

		ThrowNativeException(
			env,
			"CreateEnvironmentBlock failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);

		return (jlong)(0L);
	}

	char *dir = (char *) env->GetStringUTFChars(directory, NULL);
	char *bin = (char *) env->GetStringUTFChars(binary, NULL);
	char *cmdLine = (char *) env->GetStringUTFChars(commandLine, NULL);

	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hIn[0];
	si.hStdOutput = hOut[1];
	si.hStdError = hErr[1];

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	BOOL retval = CreateProcessAsUser(
			hToken,
			bin,
			cmdLine,
			NULL,
			NULL,
			TRUE,
			CREATE_DEFAULT_ERROR_MODE |
			CREATE_NO_WINDOW |
			DETACHED_PROCESS |
			CREATE_UNICODE_ENVIRONMENT,
			envBlk,
			dir,
			&si,
			&pi);
	if (!retval) {
		error = GetLastError();
	}

	env->ReleaseStringUTFChars(commandLine, cmdLine);
	env->ReleaseStringUTFChars(binary, bin);
	env->ReleaseStringUTFChars(directory, dir);

	DestroyEnvironmentBlock(envBlk);

	CloseHandle(hIn[0]);
	CloseHandle(hOut[1]);
	CloseHandle(hErr[1]);

	if (!retval) {
		CloseHandle(hIn[1]);
		CloseHandle(hOut[0]);
		CloseHandle(hErr[0]);

		ThrowNativeException(
			env,
			"CreateProcessAsUser failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);

		return (jlong)(0L);
	} else {
		if (pi.hThread != INVALID_HANDLE_VALUE)
			CloseHandle(pi.hThread);  

		jlong std_fds[3] = { (jlong)hIn[1], jlong(hOut[0]), jlong(hErr[0]) };
		env->SetLongArrayRegion(stdFds, 0, 3, std_fds);

		return (jlong)(pi.hProcess);
	}
}

JNIEXPORT jint JNICALL
Java_org_simplenfast_security_NTUser_getExitCode0(
	JNIEnv *env,
	jobject obj,
	jlong procId,
	jlong timeout)
{
	const char  *who = "Java_org_simplenfast_security_NTUser_getExitCode0";
	HANDLE      hProcess = (HANDLE)(procId);
	jint        ec = -1;
	DWORD       retval;
	DWORD       error;
	char        errbuf[ERRSTRLEN + 1];

	retval = WaitForSingleObject(hProcess, (DWORD)timeout);
	if (retval == WAIT_OBJECT_0) {
		GetExitCodeProcess(hProcess, &retval);
		ec = (jint)(retval);
	} else if (retval == WAIT_TIMEOUT) {
		TerminateProcess(hProcess, ERROR_PROCESS_ABORTED);
		ThrowNativeException(
			env,
			"process timed out",
			-1,
			"",
			who,
			__FILE__,
			__LINE__);
	} else {
		error = GetLastError();
		TerminateProcess(hProcess, ERROR_PROCESS_ABORTED);
		ThrowNativeException(
			env,
			"WaitForSingleObject failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);
	}

	return ec;
}

JNIEXPORT jboolean JNICALL
Java_org_simplenfast_security_NTUser_logout0(
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
