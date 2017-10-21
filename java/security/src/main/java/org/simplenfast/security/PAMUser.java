/*
 * The MIT License
 *
 * Copyright 2017 Rakesh Didwania.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

package org.simplenfast.security;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeUnit;

import javax.security.auth.login.LoginException;

/**
 * Provides authentication mechanism on Unix based platforms
 * using Pluggable Authentication Modules (or PAM).
 */
class PAMUser implements JniUser
{
	private native long login0(String serviceName, String userName, char [] password)
						throws NativeException;
	private native long execute0(String binary, String [] arguments, String directory, long [] stdFds)
						throws NativeException;
	private native int getExitCode0(long processCtx, long timeout)
						throws NativeException;
	private native boolean logout0(long context);

	private final String serviceName;
	private final String userName;
	private final char [] password;
	private long context;
	private long UID;
	private long GID;
	private long [] supplementaryGIDs;

	static {
		String libPath = System.getProperty("simplenfast.native.libpath");
		if (libPath != null) {
			System.load(libPath);
		}
	}

	/**
	 * Creates the PAMUser object and loads the native
	 * shared object as specified by system property
	 * 'simplenfast.native.libpath'.
	 * 
	 * @param serviceName - service to be used with PAM
	 * @param userName    - user name
	 * @param password    - user password
	 */
	PAMUser(String serviceName, String userName, char [] password)
	{
		this.serviceName = serviceName;
		this.userName = userName;
		this.password = password;
		this.context = 0;
		this.UID = -1;
		this.GID = -1;
		this.supplementaryGIDs = null;
	}

	private String [] toArgVector(String binary, List<String> arguments)
	{
		int size = arguments.size();
		int idx = 0;
		String [] args = new String[size + 1];

		args[idx++] = binary;

		for (String arg : arguments) {
			args[idx++] = arg;
		}

		return args;
	}

	/**
	 * Login using the credentials provided.
	 * This method calls the native method login0.
	 * 
	 * @return true if the login is successful, false otherwise.
	 * @throws LoginException
	 */
	@Override
	public synchronized boolean login()
			throws LoginException
	{
		if (context != 0) {
			return true;
		}

		try {
			context = login0(serviceName, userName, password);
		} catch (NativeException ex) {
			throw ex.toLoginException();
		}

		return (context != 0);
	}
	
	@Override
	public long execute(String binary, List<String> arguments, String directory, long [] stdFds)
		throws IOException
	{
		long processCtx = -1;
		
		try {
			processCtx = execute0(
									binary,
									toArgVector(binary, arguments),
									directory,
									stdFds);
		} catch (NativeException ex) {
			throw ex.toIOException();
		}

		return processCtx;
	}

	@Override
	public int getExitCode(long processCtx, long timeout, TimeUnit unit)
		throws InterruptedException
	{
		int ec = -1;

		if (processCtx != -1) {
			try {
				ec = getExitCode0(processCtx, unit.toSeconds(timeout));
			} catch (NativeException ex) {
				throw ex.toInterruptedException();
			}
		} else {
			throw new IllegalArgumentException("process not created yet");
		}

		return ec;
	}

	/**
	 * Logout and closes the login context.
	 * This method calls the native method logout0().
	 * 
	 * @return true if the logout is successful, false otherwise.
	 */
	@Override
	public synchronized boolean logout()
	{
		if (context != 0) {
			if (!logout0(context)) {
				return false;
			}
			context = 0;
		}
		return true;
	}

	String getUserName()
	{
		return userName;
	}

	long getUID()
	{
		return UID;
	}

	/**
	 * Called by the native code, login0().
	 * @param UID user ID
	 */
	void setUID(long UID)
	{
		this.UID = UID;
	}

	long getGID()
	{
		return GID;
	}

	/**
	 * Called by the native code, login0().
	 * @param GID primary group ID of the user
	 */
	void setGID(long GID)
	{
		this.GID = GID;
	}

	long[] getSupplementaryGIDs()
	{
		return supplementaryGIDs;
	}

	/**
	 * Called by the native code, login0().
	 * @param supplementaryGIDs array of supplementary GIDs for the user.
	 */
	void setSupplementaryGIDs(long[] supplementaryGIDs)
	{
		this.supplementaryGIDs = supplementaryGIDs;
	}
}
