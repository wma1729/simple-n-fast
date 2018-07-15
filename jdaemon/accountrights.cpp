#include <Windows.h>
#include <NTSecAPI.h>
#include <iostream>
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
	std::cerr
		<< std::endl
		<< "Add/remove rights to/from the user account."
		<< std::endl;

	std::cerr
		<< "Usage: " << progName
		<< " -account <account_name> [-add|-remove] <account_right>" << std::endl;

	std::cerr << "    <account_right> can be one of the following:" << std::endl;

	for (int i = 0; ; ++i) {
		if (Rights[i]) {
			std::cerr << "        " << Rights[i] << std::endl;
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
			std::cerr
				<< "LookupAccountName("
				<< account
				<< ") failed with status "
				<< GetLastError()
				<< std::endl;
			return 0;
		}
	}

	if (!LookupAccountName(NULL, account, sid, &sidLen, refDomain, &refDomainLen, &sidUse)) {
		std::cerr
			<< "LookupAccountName("
			<< account
			<< ") failed with status "
			<< GetLastError()
			<< std::endl;
		free(refDomain);
		free(sid);
		return 0;
	}

	if (verbose) {
		std::cout
			<< account << " ("
			<< static_cast<int>(sidUse)
			<< ") is found in reference domain "
			<< refDomain << std::endl;
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
					std::cerr
						<< "LsaRemoveAccountRights("
						<< account << ", "
						<< right << ") failed with status "
						<< LsaNtStatusToWinError(status)
						<< std::endl;
					retval = 1;
				} else if (verbose) {
					std::cout
						<< right
						<< " right is revoked from account "
						<< account << std::endl;
				}
			} else {
				status = LsaAddAccountRights(lsaHandle, sid, accountRights, 1);
				if (status != STATUS_SUCCESS) {
					std::cerr
						<< "LsaAddAccountRights("
						<< account << ", "
						<< right << ") failed with status "
						<< LsaNtStatusToWinError(status)
						<< std::endl;
					retval = 1;
				} else if (verbose) {
					std::cout
						<< right
						<< " right is granted to account "
						<< account << std::endl;
				}
			}

			delete [] rightW;

			LsaClose(lsaHandle);
		} else {
			std::cerr
				<< "LsaOpenPolicy() failed with status "
				<< LsaNtStatusToWinError(status)
				<< std::endl;
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
				std::cerr << "no account specified" << std::endl;
				return Usage(progName);
			}
		} else if (strcmp("-add", argv[i]) == 0) {
			remove = false;
			++i;
			if (argv[i]) {
				right = argv[i];
			} else {
				std::cerr << "no account right specified" << std::endl;
				return Usage(progName);
			}
		} else if (strcmp("-remove", argv[i]) == 0) {
			remove = true;
			++i;
			if (argv[i]) {
				right = argv[i];
			} else {
				std::cerr << "no account right specified" << std::endl;
				return Usage(progName);
			}
		} else if (strcmp("-verbose", argv[i]) == 0) {
			verbose = true;
		} else {
			return Usage(progName);
		}		
	}

	if (account == 0) {
		std::cerr << "no account specified" << std::endl;
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
		std::cerr
			<< "invalid right <"
			<< right
			<< "> specified" << std::endl;
		return Usage(progName);
	}

	return AddRemoveAccountRight(account, right, remove);
}
