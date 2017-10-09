package org.simplenfast.testdaemon;

import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.Logger;

/**
 * Starting Java Thread.
 *
 * This is an example worker thread. Instead of doing anything constructive, it just
 * sleeps until it is requested to stop.
 *
 * @author Rakesh Didwania
 */
public class Worker implements Runnable
{
	private static final Logger LOGGER = LogManager.getLogManager().getLogger("Daemon");
	
	@Override
	public void run()
	{
		while (true) {
			try {
				Thread.sleep(5000);
			} catch (InterruptedException ex) {
				if (DaemonNativeBridge.isTerminationRequested()) {
					LOGGER.log(Level.INFO, "termination requested");
					return;
				} else {
					LOGGER.log(Level.WARNING, "thread interrupted; continuing..");
				}
			}
		}
	}
}
