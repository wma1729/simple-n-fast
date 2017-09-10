static HANDLE                   ServiceStopEvent;
static SERVICE_STATUS_HANDLE    ServiceStatusHandle;
static SERVICE_STATUS           ServiceStatus;

/*
 * Updates the Service Control Manager. The caller can specify the state
 * and the staus. All other members of SERVICE_STATUS (initialized in
 * ServiceMain) remains the same.
 */
static VOID
UpdateScm(DWORD state, DWORD status)
{
	const char  *caller = "UpdateScm";

	ServiceStatus.dwCurrentState = state;
	ServiceStatus.dwWin32ExitCode = status;

	if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus)) {
		Log(ERR, caller, GetLastError(), "SetServiceStatus() failed");
	}

	return;
}

/*
 * Handles service controls.
 */
static VOID WINAPI
ServiceCtrlHandler(DWORD status)
{
	switch (status) {
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
			/* update service control manager */
			UpdateScm(SERVICE_STOP_PENDING, 0);

			/* stop the daemon */
			if (StopDaemon() == JNI_OK) {
				/* set the termination event */
				SetEvent(ServiceStopEvent);
			}
			break;

		default:
			break;
	}

	return;
}

/*
 * Registers service controls handler, starts the service, and executes the
 * the user function with the specified parameters.
 */
static VOID WINAPI
SeviceMain(DWORD argc, LPTSTR *argv)
{
	const char  *caller = "ServiceMain";
	DWORD       status;

	Log(INF, caller, "starting service");
	LogDaemonArgs();

	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	ServiceStatus.dwWin32ExitCode = 0;
	ServiceStatus.dwServiceSpecificExitCode = 0;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 0;

	ServiceStatusHandle = RegisterServiceCtrlHandler(
							TheDaemonArgs.name.c_str(),
							ServiceCtrlHandler);
	if ((SERVICE_STATUS_HANDLE)0 == ServiceStatusHandle) {
		Log(ERR, caller, GetLastError(), "RegisterServiceCtrlHandler() failed");
		return;
	}

	/* start java application here */
	if (StartDaemon() == JNI_OK) {

		/* update service control manager */
		UpdateScm(SERVICE_RUNNING, 0);

		Log(INF, caller, "service running");

		/* wait for the stop event */
		status = WaitForSingleObject(ServiceStopEvent, INFINITE);
		if (status != WAIT_OBJECT_0) {
			Log(ERR, caller, "WaitForSingleObject() failed with 0x%x", status);
		}
	} else {
		Log(ERR, caller, "failed to call Java code");
	}

	CloseHandle(ServiceStopEvent);
	ServiceStopEvent = 0;

	if (TheJVM) {
		TheJVM->DestroyJavaVM();
		Log(DBG, caller, "JVM destroyed");
	}

	/* update service control manager */
	UpdateScm(SERVICE_STOPPED, status);

	Log(INF, caller, "service stopped");

	delete TheLogger;

	return;
}

/**
 * Requests the service to stop by setting Global\\TerminateEvent_<name>
 * event.
 * @return 0 on success, 1 on error.
 */
static int
DaemonStop(void)
{
	const char  *caller = "DaemonStop";
	int         retval = 0;
	SC_HANDLE   hscm;
	SC_HANDLE   hsvc;

	hscm = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
	if (hscm != NULL) {
		hsvc = OpenService(hscm, TheDaemonArgs.name.c_str(), SERVICE_QUERY_STATUS | SERVICE_STOP);
		if (hsvc != NULL) {
			SERVICE_STATUS	svcStat;

			if (QueryServiceStatus(hsvc, &svcStat)) {
				if ((svcStat.dwCurrentState == SERVICE_STOP_PENDING) ||
					(svcStat.dwCurrentState == SERVICE_STOPPED)) {
					Log(DBG, caller, "already stopped or stopping");
				} else {
					if (!ControlService(hsvc, SERVICE_CONTROL_STOP, &svcStat)) {
						Log(DBG, caller, GetLastError(),
							"ControlService() failed");
						retval = 1;
					}
				}
			} else {
				Log(ERR, caller, GetLastError(),
					"QueryServiceStatus() failed");
				retval = 1;
			}
			CloseServiceHandle(hsvc);
		} else {
			Log(ERR, caller, GetLastError(),
				"OpenService() failed");
			retval = 1;
		}

		CloseServiceHandle(hscm);
	} else {
		Log(ERR, caller, GetLastError(),
			"OpenSCManager() failed");
		retval = 1;
	}

	return retval;
}

/*
 * Turns the calling process into a windows service. It does the following:
 * - Initialize the Logging.
 * - Creates the stop service event.
 * - Starts the ServiceMain() routine.
 *
 * @param name [IN] - the service name.
 * @return 0 on success, 1 on failure.
 */
int
Daemonize(void)
{
	const char  *caller = "Daemonize";
	int         len;
	char        stopEvent[MAX_PATH + 1];

	FileLogger *fileLogger = new FileLogger(TheDaemonArgs.logPath.c_str(), TheVerbosity);
	fileLogger->makeLogPath();
	TheLogger = fileLogger;

	len = snprintf(stopEvent, MAX_PATH, "Global\\TerminateEvent_%s", TheDaemonArgs.name.c_str());
	stopEvent[len] = '\0';

	ServiceStopEvent = CreateEvent(NULL, FALSE, FALSE, stopEvent);
	if (ServiceStopEvent == 0) {
		Log(ERR, caller, GetLastError(), "CreateEvent(%s) failed", stopEvent);
		return 1;
	}

	SERVICE_TABLE_ENTRY dispatchTable[] =
	{
		{ (LPSTR)TheDaemonArgs.name.c_str(), (LPSERVICE_MAIN_FUNCTION)SeviceMain },
		{ 0, 0 }
	};

	if (!StartServiceCtrlDispatcher(dispatchTable)) {
		Log(ERR, caller, GetLastError(), "StartServiceCtrlDispatcher() failed");
		return 1;
	}

	return 0;
}
