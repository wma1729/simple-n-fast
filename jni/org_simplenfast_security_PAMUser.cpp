#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include "org_simplenfast_security_PAMUser.h"
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
	int             error = 0;
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
			"malloc of passwd buffer failed",
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
				"getpwnam_r for user %s did not find any data", user);
		} else {
			snprintf(message, sizeof(message),
				"getpwnam_r for user %s failed", user);
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

	if (initgroups(pwd.pw_name, pwd.pw_gid) < 0) {
		error = errno;

		free(buf);

		snprintf(message, sizeof(message),
			"initgroups for user %s (gid %d) failed",
			pwd.pw_name, (int) (pwd.pw_gid));

		ThrowNativeException(
			env,
			message,
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
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
				"malloc of group ID buffer failed",
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
				"malloc of Java group ID buffer failed",
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
Java_org_simplenfast_security_PAMUser_login0(
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

static bool
CreatePipe(
	JNIEnv *env,
	int fd[],
	bool readInheritable,
	bool writeInheritable)
{
	const char  *who = "CreatePipe";
	char        errbuf[ERRSTRLEN + 1];
	int         f[2];

	if (pipe(f) < 0) {
		int error = errno;

		ThrowNativeException(
			env,
			"pipe failed",
			error,
			GetErrorStr(errbuf, ERRSTRLEN, error),
			who,
			__FILE__,
			__LINE__);

		return false;
	}

	if (!readInheritable) {
		if (fcntl(f[0], F_SETFD, FD_CLOEXEC) < 0) {
			int error = errno;

			close(f[0]);
			close(f[1]);

			ThrowNativeException(
				env,
				"fcntl failed to set FD_CLOEXEC flag on read handle",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);

			return false;
		}
	}

	if (!writeInheritable) {
		if (fcntl(f[1], F_SETFD, FD_CLOEXEC) < 0) {
			int error = errno;

			close(f[0]);
			close(f[1]);

			ThrowNativeException(
				env,
				"fcntl failed to set FD_CLOEXEC flag on write handle",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);

			return false;
		}
	}

	fd[0] = f[0];
	fd[1] = f[1];

	return true;
}

JNIEXPORT jlong JNICALL
Java_org_simplenfast_security_PAMUser_execute0(
	JNIEnv *env,
	jobject obj,
	jstring binary,
	jobjectArray arguments,
	jstring directory,
	jlongArray stdFds)
{
	const char  *who = "Java_org_simplenfast_security_PAMUser_execute0";
	int         error = 0;
	int         std_in[2];
	int         std_out[2];
	int         std_err[2];
	uid_t       uid = -1;
	gid_t       gid = -1;
	const char  *userName;
	jstring     usr;
	jclass      cls;
	jmethodID   mid;
	char        message[512];
	char        errbuf[ERRSTRLEN + 1];

	cls = env->GetObjectClass(obj);

	mid = env->GetMethodID(
			cls,
			"getUID",
			"()J");
	uid = (uid_t) env->CallLongMethod(obj, mid);

	mid = env->GetMethodID(
			cls,
			"getUserName",
			"()Ljava/lang/String;");
	usr = (jstring) env->CallObjectMethod(obj, mid);
	userName = env->GetStringUTFChars(usr, 0);

	mid = env->GetMethodID(
			cls,
			"getGID",
			"()J");
	gid = (gid_t) env->CallLongMethod(obj, mid);

	if (!CreatePipe(env, std_in, true, false)) {
		return (jlong)(-1L);
	}

	if (!CreatePipe(env, std_out, false, true)) {
		close(std_in[0]);
		close(std_in[1]);
		return (jlong)(-1L);
	}

	if (!CreatePipe(env, std_err, false, true)) {
		close(std_in[0]);
		close(std_in[1]);
		close(std_out[0]);
		close(std_out[1]);
		return (jlong)(-1L);
	}

	pid_t pid = fork();
	if (pid == 0) {
		dup2(std_in[0], 0);
		close(std_in[0]);
		close(std_in[1]);

		dup2(std_out[1], 1);
		close(std_out[1]);
		close(std_out[0]);

		dup2(std_err[1], 2);
		close(std_err[1]);
		close(std_err[0]);

		if (initgroups(userName, gid) < 0) {
			error = errno;
			size_t n = snprintf(message, sizeof(message),
					"initgroups for %s:%d failed: %s (%d)",
					userName,
					(int) gid,
					GetErrorStr(errbuf, ERRSTRLEN, error),
					error);
			n = write(2, message, n);
			exit(error);
		}

		env->ReleaseStringUTFChars(usr, userName);

		if (setgid(gid) < 0) {
			error = errno;
			size_t n = snprintf(message, sizeof(message),
					"setgid to %d failed: %s (%d)",
					(int) gid,
					GetErrorStr(errbuf, ERRSTRLEN, error),
					error);
			n = write(2, message, n);
			exit(error);
		}

		if (setuid(uid) < 0) {
			error = errno;
			size_t n = snprintf(message, sizeof(message),
					"setuid to %d failed: %s (%d)",
					(int) uid,
					GetErrorStr(errbuf, ERRSTRLEN, error),
					error);
			n = write(2, message, n);
			exit(error);
		}

		char *dir = (char *) env->GetStringUTFChars(directory, NULL);
		if (dir && *dir) {
			if (chdir(dir) < 0) {
				error = errno;
				size_t n = snprintf(message, sizeof(message),
						"chdir to %s failed: %s (%d)",
						dir,
						GetErrorStr(errbuf, ERRSTRLEN, error),
						error);
				n = write(2, message, n);
				exit(error);
			}
			env->ReleaseStringUTFChars(directory, dir);
		}

		char *bin = (char *) env->GetStringUTFChars(binary, NULL);
		jsize argLen = env->GetArrayLength(arguments);
		char **args = (char **)calloc(argLen + 1, sizeof(char *));
		for (jsize i = 0; i < argLen; ++i) {
			jstring arg = (jstring) env->GetObjectArrayElement(arguments, i);
			args[i] = (char *) env->GetStringUTFChars(arg, NULL);
		}

		if (execvp(bin, args) < 0) {
			error = errno;

			size_t n = snprintf(message, sizeof(message),
					"exec of %s failed: %s (%d)",
					bin,
					GetErrorStr(errbuf, ERRSTRLEN, error),
					error);
			n = write(2, message, n);
			exit(error);
		}
	} else if (pid > 0) {
		close(std_in[0]);
		close(std_out[1]);
		close(std_err[1]);

		jlong std_fds[3] = { (jlong)std_in[1], jlong(std_out[0]), jlong(std_err[0]) };
		env->SetLongArrayRegion(stdFds, 0, 3, std_fds);
	} else {
		close(std_in[0]);
		close(std_in[1]);
		close(std_out[0]);
		close(std_out[1]);
		close(std_err[0]);
		close(std_err[1]);

		ThrowNativeException(
			env,
			"fork failed",
			errno,
			GetErrorStr(errbuf, ERRSTRLEN, errno),
			who,
			__FILE__,
			__LINE__);
	}

	return (jlong)pid;
}

JNIEXPORT jint JNICALL
Java_org_simplenfast_security_PAMUser_getExitCode0(
	JNIEnv *env,
	jobject obj,
	jlong procId,
	jlong timeout)
{
	const char  *who = "Java_org_simplenfast_security_PAMUser_getExitCode0";
	bool        done = false;
	jint        ec = -1;
	int         error = 0;
	pid_t       pid = (pid_t)procId;
	char        errbuf[ERRSTRLEN + 1];

	while ((timeout > 0) && !done) {
		int cstat;
		pid_t rpid = waitpid(pid, &cstat, WNOHANG);
		if (rpid == pid) {
			ec = WEXITSTATUS(cstat);
			done = true;
		} else if (rpid == 0) {
			sleep(1);
			timeout--;
		} else if (rpid < 0) {
			error = errno;
			kill(pid, SIGTERM);
			ThrowNativeException(
				env,
				"waitpid failed",
				error,
				GetErrorStr(errbuf, ERRSTRLEN, error),
				who,
				__FILE__,
				__LINE__);
			done = true;
		}
	}

	if (!done) {
		kill(pid, SIGTERM);
		ThrowNativeException(
			env,
			"process timed out",
			-1,
			"",
			who,
			__FILE__,
			__LINE__);
	}

	return ec;
}

JNIEXPORT jboolean JNICALL
Java_org_simplenfast_security_PAMUser_logout0(
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
