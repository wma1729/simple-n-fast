#include <Windows.h>
#include <NTSecAPI.h>
#include <cstdio>
#include <cstring>
#include "common.h"
#include "i18n.h"

#if !defined(STATUS_SUCCESS)
#define STATUS_SUCCESS 0
#endif

static bool verbose = false;

static const char *Rights[] = {
	SE_INTERACTIVE_LOGON_NAME,
	SE_DENY_INTERACTIVE_LOGON_NAME,
	SE_NETWORK_LOGON_NAME,
	SE_DENY_NETWORK_LOGON_NAME,
	SE_BATCH_LOGON_NAME,
	SE_DENY_BATCH_LOGON_NAME,
	SE_SERVICE_LOGON_NAME,
	SE_DENY_SERVICE_LOGON_NAME,
	SE_REMOTE_INTERACTIVE_LOGON_NAME,
	SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME,
	0
};

static int
Usage(const char *progName)
{
	fprintf(stderr, "\nAdd/remove rights to/from the user account.\n");
	fprintf(stderr, "Usage: %s -account <account_name> [-add|-remove] <account_right>\n", progName);
	fprintf(stderr, "    <account_right> can be one of the following:\n");

	for (int i = 0; ; ++i) {
		if (Rights[i]) {
			fprintf(stderr, "        %s\n", Rights[i]);
		} else {
			break;
		}
	}

	return 1;
}

static PSID
GetAccountSid(const char *account)
{
	PSID sid = 0;
	DWORD sidLen = 0;
	char *refDomain = 0;
	DWORD refDomainLen = 0;
	SID_NAME_USE sidUse;

	if (!LookupAccountName(NULL, account, sid, &sidLen, refDomain, &refDomainLen, &sidUse)) {
		if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			sid = (PSID)malloc(sidLen);
			refDomain = (char *)malloc(refDomainLen);
		} else {
			fprintf(stderr, "LookupAccountName(%s) failed with status %d\n",
					account, GetLastError());
			return 0;
		}
	}

	if (!LookupAccountName(NULL, account, sid, &sidLen, refDomain, &refDomainLen, &sidUse)) {
		fprintf(stderr, "LookupAccountName(%s) failed with status %d\n",
				account, GetLastError());
		free(refDomain);
		free(sid);
		return 0;
	}

	if (verbose) {
		fprintf(stdout, "%s (%d) is found in reference domain %s\n",
				account, int(sidUse), refDomain);
	}

	free(refDomain);
	return sid;
}

static int
AddRemoveAccountRight(const char *account, const char *right, bool remove)
{
	int retval = 0;
	PSID sid = GetAccountSid(account);

	if (sid) {
		NTSTATUS status = 0;
		LSA_OBJECT_ATTRIBUTES attr;
		LSA_HANDLE lsaHandle;

		ZeroMemory(&attr, sizeof(attr));

		status = LsaOpenPolicy(NULL, &attr, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &lsaHandle);
		if (status == STATUS_SUCCESS) {
			size_t len = strlen(right);
			wchar_t *rightW = snf::mbs2wcs(right);

			LSA_UNICODE_STRING accountRights[1];
			accountRights[0].Length = (USHORT)(len * sizeof(wchar_t));
			accountRights[0].MaximumLength = (USHORT)(accountRights[0].Length + sizeof(wchar_t));
			accountRights[0].Buffer = (PWSTR)rightW;

			if (remove) {
				status = LsaRemoveAccountRights(lsaHandle, sid, FALSE, accountRights, 1);
				if (status != STATUS_SUCCESS) {
					fprintf(stderr, "LsaRemoveAccountRights(%s, %s) failed with status %d\n",
							account, right, LsaNtStatusToWinError(status));
					retval = 1;
				} else if (verbose) {
					fprintf(stdout, "%s right is revoked from account %s.\n",
							right, account);
				}
			} else {
				status = LsaAddAccountRights(lsaHandle, sid, accountRights, 1);
				if (status != STATUS_SUCCESS) {
					fprintf(stderr, "LsaAddAccountRights(%s, %s) failed with status %d\n",
							account, right, LsaNtStatusToWinError(status));
					retval = 1;
				} else if (verbose) {
					fprintf(stdout, "%s right is granted to account %s.\n",
							right, account);
				}
			}

			delete [] rightW;

			LsaClose(lsaHandle);
		} else {
			fprintf(stderr, "LsaOpenPolicy() failed with status %d\n",
				LsaNtStatusToWinError(status));
			retval = 1;
		}


		free(sid);
	}

	return retval;
}

int
main(int argc, const char **argv)
{
	char progName[MAXPATHLEN + 1];
	bool remove = false;
	const char *account = 0;
	const char *right = 0;

	snf::basename(progName, MAXPATHLEN, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcmp("-account", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				account = argv[i];
			} else {
				fprintf(stderr, "%s\n", "no account specified");
				return Usage(progName);
			}
		} else if (strcmp("-add", argv[i]) == 0) {
			remove = false;
			++i;
			if (argv[i]) {
				right = argv[i];
			} else {
				fprintf(stderr, "%s\n", "no account right specified");
				return Usage(progName);
			}
		} else if (strcmp("-remove", argv[i]) == 0) {
			remove = true;
			++i;
			if (argv[i]) {
				right = argv[i];
			} else {
				fprintf(stderr, "%s\n", "no account right specified");
				return Usage(progName);
			}
		} else if (strcmp("-verbose", argv[i]) == 0) {
			verbose = true;
		} else {
			return Usage(progName);
		}		
	}

	if (account == 0) {
		fprintf(stderr, "%s\n", "no account specified");
		return Usage(progName);
	}

	bool valid = false;

	for (int i = 0; ; ++i) {
		if (Rights[i]) {
			if (strcmp(right, Rights[i]) == 0) {
				valid = true;
				break;
			}
		} else {
			break;
		}
	}

	if (!valid) {
		fprintf(stderr, "invalid right <%s> specified\n", right);
		return Usage(progName);
	}

	return AddRemoveAccountRight(account, right, remove);
}
