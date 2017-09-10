#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

static sigset_t DaemonSignalSet;

extern "C" {

/*
 * Signal handler thread for the daemon. Waits for one of the following
 * signals:
 * - SIGINT
 * - SIGTERM
 * - SIGQUIT
 * Once the signal is received, the stop method in the stop class in
 * the Java code is invoked and then the thread exits.
 */
static void *
DaemonSignalHandler(void *)
{
	int signo;
	int stop = 0;

	while (1) {
		sigwait(&DaemonSignalSet, &signo);

		switch (signo) {
			case SIGINT:
			case SIGTERM:
			case SIGQUIT:
				stop = 1;
				break;

			default:
				break;
		}

		if (stop) {
			if (StopDaemon() == JNI_OK) {
				break;
			}
		}
	}

	return 0;
}

}

/*
 * Checks if another instance of the daemon is already running. The running
 * daemon keeps a lock on pidPath. Please note that 'fd' is not closed.
 * This is intentional. Must be called after the process has daemonized
 * itself.
 *
 * @param [out] running - set to true if another instance of the daemon
 *                        is running, set to false otherwise.
 *
 * @return 0 on success, non-zero on failure.
 */
static int
DaemonAlreadyRunning(bool *running)
{
	const char      *caller = "DaemonAlreadyRunning";
	int             retval = 0;
	int             fd;
	struct flock    fl;

	*running = false;

	fd = open(TheDaemonArgs.pidPath.c_str(), O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		Log(ERR, caller, errno, "open(%s) failed",
			TheDaemonArgs.pidPath.c_str());
		return 1;
	} else {
		fl.l_type = F_WRLCK;
		fl.l_whence = SEEK_SET;
		fl.l_start = 0;
		fl.l_len = 0;

		if (fcntl(fd, F_SETLK, &fl) < 0) {
			if ((errno == EACCES) || (errno == EAGAIN)) {
				*running = true;
				retval = 0;
			} else {
				Log(ERR, caller, errno, "lock(%s) failed",
					TheDaemonArgs.pidPath.c_str());
				retval = 1;
			}
			close(fd);
		} else {
			char	buf[32];

			if (ftruncate(fd, 0) < 0) {
				Log(ERR, caller, errno, "ftruncate() failed");
				retval = 1;
			} else {
				int nbytes = snprintf(buf, sizeof(buf) - 1, "%d", getpid());
				buf[nbytes] = '\0';			

				if (write(fd, buf, nbytes) < 0) {
					Log(ERR, caller, errno, "write() failed");
				}
			}

			// keep fd open; fd leak is justified.
		}
	}

	return retval;
}

/*
 * Sends SIGTERM signal to the daemon. The process' pid is obtained from 
 * the pidPath.
 *
 * @return 0 on success, non-zero on failure.
 */ 
static int
DaemonStop(void)
{
	const char  *caller = "DaemonStop";
	int         retval = 0;
	FILE        *fp = 0;
	int         i, pid;

	fp = fopen(TheDaemonArgs.pidPath.c_str(), "r");
	if (fp) {
		i = fscanf(fp, "%d", &pid);
		if (i != 1) {
			Log(ERR, caller, errno,
				"unable to read pid from file %s",
				TheDaemonArgs.pidPath.c_str());
			retval = 1;
		} else {
			if (kill(pid, SIGTERM) < 0) {
				Log(ERR, caller, errno,
					"unable to kill process %d", pid);
				retval = 1;
			}
		}

		fclose(fp);
	} else {
		Log(ERR, caller, errno,
			"unable to open file %s",
			TheDaemonArgs.pidPath.c_str());
		retval = 1;
	}

	return retval;
}

/*
 * Turns the calling process into a daemon process. It does the following:
 * - Sets the umask to 0.
 * - Forks and let the parent exit.
 * - Creates a new session (setsid).
 * - Change directory to the home directory.
 * - Close all the open file descriptors (0, 1, and 2 are most important).
 * - Initialize the Logging.
 * - Check if only a single instance of the daemon is running.
 * - Blocks SIGINT, SIGTERM, SIGQUIT signal.
 * - Starts another thread to handle those events.
 * - Use JNI invoke APIs to load JVM.
 * - Calls the start method in the start class in the Java code.
 *
 * @return 0 on success, non-zero on failure.
 */
static int
Daemonize(void)
{
	const char      *caller = "Daemonize";
	int             retval = 0;
	bool            alreadyRunning;
	pid_t           pid;
	struct rlimit   rl;
	pthread_t       tid;

	umask(0);

	pid = fork();
	if (pid < 0) {
		Log(ERR, caller, errno, "fork() failed");
		return 1;
	}

	if (pid != 0)
		exit(0);

	if (setsid() < 0) {
		Log(ERR, caller, errno, "setsid() failed");
		return 1;
	}

	if (chdir(TheDaemonArgs.home.c_str()) < 0) {
		Log(ERR, caller, errno, "chdir(%s) failed",
			TheDaemonArgs.home.c_str());
		return 1;
	}

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		Log(ERR, caller, errno, "getrlimit() failed");
		return 1;
	}

	if (rl.rlim_max == RLIM_INFINITY) {
		rl.rlim_max = 1024;
	}

	for (rlim_t r = 0; r < rl.rlim_max; ++r) {
		close((int)r);
	}

	FileLogger *fileLogger = new FileLogger(TheDaemonArgs.logPath.c_str(), TheVerbosity);
	fileLogger->makeLogPath();
	TheLogger = fileLogger;

	Log(INF, caller, "starting daemon");
	LogDaemonArgs();

	retval = DaemonAlreadyRunning(&alreadyRunning);
	if (retval != 0) {
		return retval;
	} else if (alreadyRunning) {
		Log(WRN, caller, "another instance of %s already running",
			TheDaemonArgs.name.c_str());
		return 1;
	}

	sigemptyset(&DaemonSignalSet);
	sigaddset(&DaemonSignalSet, SIGINT);
	sigaddset(&DaemonSignalSet, SIGTERM);
	sigaddset(&DaemonSignalSet, SIGQUIT);

	retval = pthread_sigmask(SIG_BLOCK, &DaemonSignalSet, NULL);
	if (retval != 0) {
		Log(ERR, caller, retval, "pthread_sigmask() failed");
		return 1;
	}

	retval = pthread_create(&tid, 0, DaemonSignalHandler, 0);
	if (retval != 0) {
		Log(ERR, caller, retval, "pthread_create() failed");
		return 1;
	}

	if (StartDaemon() == JNI_OK) {
		Log(INF, caller, "daemon running");
	} else {
		Log(ERR, caller, "failed to call Java code");
		kill(getpid(), SIGTERM);
	}

	retval = pthread_join(tid, 0);
	if (retval != 0) {
		Log(ERR, caller, retval, "pthread_join() failed");
		return 1;
	}

	if (TheJVM) {
		TheJVM->DestroyJavaVM();
		Log(DBG, caller, "JVM destroyed");
	}

	Log(INF, caller, "daemon stopped");

	delete TheLogger;

	return 0;
}
