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

package org.simplenfast.jaas;

import org.simplenfast.nex.NativeException;
import org.simplenfast.nex.NativeUtil;

import javax.security.auth.login.LoginException;

/**
 * Provides authentication mechanism on Unix based platforms
 * using Pluggable Authentication Modules (or PAM).
 */
public class PAMUser
{
	private native long login0(String serviceName, String userName, char [] password)
						throws NativeException;
	private native boolean logout0(long context);

	private final String serviceName;
	private final String userName;
	private final char [] password;
	private long context;
	private long UID;
	private long GID;
	private long [] supplementaryGIDs;

	/**
	 * Creates the PAMUser object and loads the native
	 * shared object as specified by system property
	 * 'simplenfast.jaas.libpath'.
	 * 
	 * @param serviceName - service to be used with PAM
	 * @param userName    - user name
	 * @param password    - user password
	 */
	public PAMUser(String serviceName, String userName, char [] password)
	{
		this.serviceName = serviceName;
		this.userName = userName;
		this.password = password;
		this.context = 0;
		this.UID = -1;
		this.GID = -1;
		this.supplementaryGIDs = null;

		String libPath = System.getProperty("simplenfast.jaas.libpath");
		if (libPath != null) {
			System.load(libPath);
		}
	}

	/**
	 * Login using the credentials provided.
	 * This method calls the native method login0.
	 * 
	 * @return true if the login is successful, false otherwise.
	 * @throws LoginException
	 */
	public synchronized boolean login()
			throws LoginException
	{
		if (context != 0) {
			return true;
		}

		try {
			context = login0(serviceName, userName, password);
		} catch (NativeException ex) {
			throw NativeUtil.NativeToLoginException(ex);
		}

		return (context != 0);
	}

	/**
	 * Logout and closes the login context.
	 * This method calls the native method logout0().
	 * 
	 * @return true if the logout is successful, false otherwise.
	 */
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

	public String getUserName()
	{
		return userName;
	}

	public long getUID()
	{
		return UID;
	}

	/**
	 * Called by the native code, login0().
	 * @param UID user ID
	 */
	public void setUID(long UID)
	{
		this.UID = UID;
	}

	public long getGID()
	{
		return GID;
	}

	/**
	 * Called by the native code, login0().
	 * @param GID primary group ID of the user
	 */
	public void setGID(long GID)
	{
		this.GID = GID;
	}

	public long[] getSupplementaryGIDs()
	{
		return supplementaryGIDs;
	}

	/**
	 * Called by the native code, login0().
	 * @param supplementaryGIDs array of supplementary GIDs for the user.
	 */
	public void setSupplementaryGIDs(long[] supplementaryGIDs)
	{
		this.supplementaryGIDs = supplementaryGIDs;
	}
}
