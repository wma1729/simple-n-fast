package org.simplenfast.testdaemon;

import java.io.File;
import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

/**
 * Bridge between the native and the Java code.
 *
 * This is an example bridge class between the native and the Java code.
 * Please focus on the conciseness of the start and stop methods.
 *
 * @author Rakesh Didwania
 */
public class DaemonNativeBridge
{
	private static final Logger LOGGER = Logger.getLogger("Daemon");
	private static boolean terminationRequested = false;
	private static Thread workerThread;

	/*
	 * Initialize logging. This will help in understanding the example better.
	 * Look at config.[unix|win] to see how demon.logPath is specified.
	 */
	static {
		String logPath = System.getProperty("daemon.logPath");
		if (logPath != null) {
			try {
				String logFile = logPath + File.separator + "daemon.log";
				Handler fh = new FileHandler(logFile);
				fh.setFormatter(new SimpleFormatter());
				LOGGER.addHandler(fh);
			} catch (IOException | SecurityException ex) {
				LOGGER.log(Level.SEVERE, "failed to add file handler", ex);
			}
		}
	}

	/**
	 * Is termination requested? This is what the Java code should look at to decide if
	 * it is time to stop.
	 *
	 * @return true if it is time to stop, false otherwise.
	 */
	public static boolean isTerminationRequested()
	{
		return terminationRequested;
	}

	/**
	 * Entry point into the Java code.
	 *
	 * Here we create a worker thread, start it and return immediately. From this point
	 * onward, you are almost entirely working in the Java world.
	 */
	public static void start()
	{
		LOGGER.log(Level.INFO, "start daemon invoked");
		workerThread = new Thread(new Worker());
		workerThread.start();
		LOGGER.log(Level.INFO, "worker thread started");
	}

	/**
	 * Requests the Java code to stop.
	 *
	 * All we do here is set a flag and interrupt the worker thread (optional) and
	 * return immediately.
	 */
	public static void stop()
	{
		LOGGER.log(Level.INFO, "stop daemon invoked");
		terminationRequested = true;
		if ((workerThread != null) && workerThread.isAlive()) {
			workerThread.interrupt();
			LOGGER.log(Level.INFO, "worker thread interrupted");
		}
	}
}
