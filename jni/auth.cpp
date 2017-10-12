#include <cstdlib>
#include <cstring>
#include "common.h"
#include "auth.h"
#include "ex.h"
#include "i18n.h"

char *
jcharArrayToCString(
	JNIEnv *env,
	jcharArray jarr)
{
	const char  *who = "jcharArrayToCString";
	jsize       arrayLen = env->GetArrayLength(jarr);
	jchar       *array = env->GetCharArrayElements(jarr, NULL);
	wchar_t     *dataW;
	char        *data;

	dataW = (wchar_t *)calloc(arrayLen + 1, sizeof(wchar_t));
	if (dataW == 0) {
		char errbuf[ERRSTRLEN + 1];

		ThrowNativeException(
			env,
			"malloc() of array buffer failed",
			errno,
			GetErrorStr(errbuf, ERRSTRLEN, errno),
			who,
			__FILE__,
			__LINE__);

		env->ReleaseCharArrayElements(jarr, array, 0);
		return 0;
	}

	for (jsize i = 0; i < arrayLen; ++i) {
		dataW[i] = (wchar_t)(array[i]);
	}

	env->ReleaseCharArrayElements(jarr, array, 0);

	data = WcsToMbs(dataW);
	free(dataW);
	return data;
}

#if defined(WINDOWS)

HANDLE
nt_login(
	JNIEnv *env,
	const char *domain,
	const char *user,
	const char *password)
{
	const char  *who = "nt_login";
	DWORD       error = 0;
	HANDLE      hToken = NULL;
	char        message[512];
	char        errbuf[ERRSTRLEN + 1];

	if (!LogonUser(
			user,
			domain,
			password,
			LOGON32_LOGON_INTERACTIVE,
			LOGON32_PROVIDER_DEFAULT,
			&hToken)) {

		error = GetLastError();

		snprintf(message, sizeof(message),
				"LogonUser() for %s\\%s failed",
				domain,
				user);

		ThrowNativeException(
			env,
			message,
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);

	} else {
		if (!ImpersonateLoggedOnUser(hToken)) {
			error = GetLastError();
			CloseHandle(hToken);
			hToken = NULL;

			snprintf(message, sizeof(message),
					"ImpersonateLoggedOnUser() for %s\\%s failed",
					domain,
					user);

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

	return hToken;
}

bool
nt_logout(HANDLE hToken)
{
	if (TRUE == RevertToSelf()) {
		CloseHandle(hToken);
		return true;
	}
	return false;
}

#else // !WINDOWS

extern "C" {

static int
conversation(
	int nmsg,
#if defined(LINUX)
	const
#endif
	struct pam_message **msg,
	struct pam_response **resp,
	void *p_app_data)
{
	const char *pwd = 0;
	struct pam_response *reply = 0;

	*resp = 0;

	if (p_app_data == 0) {
		return PAM_CONV_ERR;
	} else {
		pwd = (const char *)p_app_data;
	}

	reply = (struct pam_response *)malloc(sizeof(struct pam_response));
	if (reply == 0) {
		return PAM_CONV_ERR;
	}

	reply->resp = strdup(pwd);
	reply->resp_retcode = PAM_SUCCESS;

	*resp = reply;

	return PAM_SUCCESS;
}

}

bool
pam_login(
	JNIEnv *env,
	const char *service,
	const char *user,
	void *password,
	pam_handle_t **pamh)
{
	const char  *who = "pam_login";
	int         retval = PAM_PERM_DENIED;
	char        message[512];
	const struct pam_conv conv = { conversation, password };

	if (pamh) {
		*pamh = 0;
	}

	do {
		retval = pam_start(service, user, &conv, pamh);
		if (retval != PAM_SUCCESS) {
			snprintf(message, sizeof(message),
				"pam_start() for user %s failed using service %s",
				user, service);

			ThrowNativeException(
				env,
				message,
				retval,
				pam_strerror(0, retval),
				who,
				__FILE__,
				__LINE__);

			*pamh = 0;
			break;
		}

		retval = pam_authenticate(*pamh, 0);
		if (retval != PAM_SUCCESS) {
			snprintf(message, sizeof(message),
				"pam_authenticate() for user %s failed using service %s",
				user, service);

			ThrowNativeException(
				env,
				message,
				retval,
				pam_strerror(*pamh, retval),
				who,
				__FILE__,
				__LINE__);

			break;
		}

		retval = pam_acct_mgmt(*pamh, 0);
		if (retval != PAM_SUCCESS) {
			snprintf(message, sizeof(message),
				"pam_acct_mgmt() for user %s failed using service %s",
				user, service);

			ThrowNativeException(
				env,
				message,
				retval,
				pam_strerror(*pamh, retval),
				who,
				__FILE__,
				__LINE__);

			break;
		}
	} while (0);

	if (retval != PAM_SUCCESS) {
		if (*pamh) {
			pam_end(*pamh, retval);
		}
		return false;
	}

	return true;
}

bool
pam_logout(pam_handle_t *pamh)
{
	if (pamh) {
		int retval = pam_end(pamh, PAM_SUCCESS);
		if (retval == PAM_SUCCESS) {
			return true;
		}
	}

	return false;
}

#endif
