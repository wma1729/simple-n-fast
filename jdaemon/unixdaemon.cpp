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
	int             retval = 0;
	int             fd;
	struct flock    fl;

	*running = false;

	fd = open(TheDaemonArgs.pidPath.c_str(), O_CREAT | O_RDWR, 0644);
	if (fd < 0) {
		ERROR_STRM(nullptr, errno)
			<< "open(" << TheDaemonArgs.pidPath << ") failed"
			<< snf::log::record::endl;
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
				ERROR_STRM(nullptr, errno)
					<< "lock(" << TheDaemonArgs.pidPath << ") failed"
					<< snf::log::record::endl;
				retval = 1;
			}
			close(fd);
		} else {
			char	buf[32];

			if (ftruncate(fd, 0) < 0) {
				ERROR_STRM(nullptr, errno)
					<< "ftruncate(" << TheDaemonArgs.pidPath << ") failed"
					<< snf::log::record::endl;
				retval = 1;
			} else {
				int nbytes = snprintf(buf, sizeof(buf) - 1, "%d", getpid());
				buf[nbytes] = '\0';			

				if (write(fd, buf, nbytes) < 0) {
					ERROR_STRM(nullptr, errno)
						<< "write(" << TheDaemonArgs.pidPath << ") failed"
						<< snf::log::record::endl;
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
	int     retval = 0;
	FILE    *fp = 0;
	int     i, pid;

	fp = fopen(TheDaemonArgs.pidPath.c_str(), "r");
	if (fp) {
		i = fscanf(fp, "%d", &pid);
		if (i != 1) {
			ERROR_STRM(nullptr, errno)
				<< "unable to read pid from file "
				<< TheDaemonArgs.pidPath
				<< snf::log::record::endl;
			retval = 1;
		} else {
			if (kill(pid, SIGTERM) < 0) {
				ERROR_STRM(nullptr, errno)
					<< "unable to kill process " << pid
					<< snf::log::record::endl;
				retval = 1;
			}
		}

		fclose(fp);
	} else {
		ERROR_STRM(nullptr, errno)
			<< "unable to open file " << TheDaemonArgs.pidPath
			<< snf::log::record::endl;
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
	int             retval = 0;
	int             fd = -1;
	bool            alreadyRunning;
	pid_t           pid;
	struct rlimit   rl;
	pthread_t       tid;

	umask(0);

	pid = fork();
	if (pid < 0) {
		ERROR_STRM(nullptr, errno)
			<< "fork() failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (pid != 0)
		exit(0);

	if (setsid() < 0) {
		ERROR_STRM(nullptr, errno)
			<< "setsid() failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (chdir(TheDaemonArgs.home.c_str()) < 0) {
		ERROR_STRM(nullptr, errno)
			<< "chdir(" << TheDaemonArgs.home << ") failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		ERROR_STRM(nullptr, errno)
			<< "getrlimit() failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (rl.rlim_max == RLIM_INFINITY) {
		rl.rlim_max = 1024;
	}

	for (rlim_t r = 0; r < rl.rlim_max; ++r) {
		close((int)r);
	}

	// Attach stdin, stdout, stderr to /dev/null
	fd = open("/dev/null", O_RDWR);
	dup2(fd, 1);
	dup2(fd, 2);

	AddFileLogger();

	INFO_STRM(nullptr)
		<< "starting daemon"
		<< snf::log::record::endl;

	LogDaemonArgs();

	retval = DaemonAlreadyRunning(&alreadyRunning);
	if (retval != 0) {
		return retval;
	} else if (alreadyRunning) {
		WARNING_STRM(nullptr)
			<< "another instance of "
			<< TheDaemonArgs.name
			<< " is already running"
			<< snf::log::record::endl;
		return 1;
	}

	sigemptyset(&DaemonSignalSet);
	sigaddset(&DaemonSignalSet, SIGINT);
	sigaddset(&DaemonSignalSet, SIGTERM);
	sigaddset(&DaemonSignalSet, SIGQUIT);

	retval = pthread_sigmask(SIG_BLOCK, &DaemonSignalSet, NULL);
	if (retval != 0) {
		ERROR_STRM(nullptr, retval)
			<< "pthread_sigmask() failed"
			<< snf::log::record::endl;
		return 1;
	}

	retval = pthread_create(&tid, 0, DaemonSignalHandler, 0);
	if (retval != 0) {
		ERROR_STRM(nullptr, retval)
			<< "pthread_create() failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (StartDaemon() == JNI_OK) {
		INFO_STRM(nullptr)
			<< "daemon running"
			<< snf::log::record::endl;

	} else {
		ERROR_STRM(nullptr)
			<< "failed to call Java code"
			<< snf::log::record::endl;
		pthread_kill(tid, SIGTERM);
	}

	retval = pthread_join(tid, 0);
	if (retval != 0) {
		ERROR_STRM(nullptr, retval)
			<< "pthread_join() failed"
			<< snf::log::record::endl;
		return 1;
	}

	if (TheJVM) {
		TheJVM->DestroyJavaVM();
		DEBUG_STRM(nullptr)
			<< "JVM destroyed"
			<< snf::log::record::endl;
	}

	INFO_STRM(nullptr)
		<< "daemon stopped"
		<< snf::log::record::endl;

	return 0;
}
