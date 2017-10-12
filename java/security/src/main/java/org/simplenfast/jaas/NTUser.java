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
 * Provides authentication mechanism on Windows platforms
 * using Win32 APIs (LogonUser & ImpersonateLoggedOnUser for
 * login, and RevertToSelf for logout).
 */
public class NTUser
{
	private native long login0(String domainName, String userName, char [] password)
						throws NativeException;
	private native boolean logout0(long impersonationToken);

	private final String domainName;
	private final String userName;
	private final char [] password;
	private String userSID;
	private String domainSID;
	private String primaryGroupSID;
	private String [] groupSIDs;
	private long impersonationToken;

	/**
	 * Creates the NTUser object and loads the native
	 * DLL as specified by system property
	 * 'simplenfast.jaas.libpath'.
	 * 
	 * @param user     - user name
	 * @param password - user password
	 */
	public NTUser(String user, char [] password)
	{
		String [] fields = user.split("\\\\");
		switch (fields.length) {
			case 2:
				this.domainName = fields[0];
				this.userName = fields[1];
				break;
			case 1:
				this.domainName = ".";
				this.userName = fields[0];
				break;
			default:
				throw new IllegalArgumentException("invalid user " + user);
		}

		this.password = password;
		this.userSID = null;
		this.primaryGroupSID = null;
		this.groupSIDs = null;
		this.impersonationToken = 0;

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
		if (impersonationToken != 0) {
			return true;
		}

		try {
			impersonationToken = login0(domainName, userName, password);
		} catch (NativeException ex) {
			throw NativeUtil.NativeToLoginException(ex);
		}

		return (impersonationToken != 0);
	}

	/**
	 * Logout and closes the login context.
	 * This method calls the native method logout0().
	 * 
	 * @return true if the logout is successful, false otherwise.
	 */
	public synchronized boolean logout()
	{
		if (impersonationToken != 0) {
			if (!logout0(impersonationToken)) {
				return false;
			}
			impersonationToken = 0;
		}

		return true;
	}

	public String getUserName()
	{
		return userName;
	}

	public String getDomainName()
	{
		return domainName;
	}

	public String getUserSID()
	{
		return userSID;
	}

	/**
	 * Called by native code, login0().
	 * @param userSID user SID
	 */
	public void setUserSID(String userSID)
	{
		this.userSID = userSID;
	}

	public String getDomainSID()
	{
		return domainSID;
	}

	/**
	 * Called by native code, login0().
	 * @param domainSID domain SID
	 */
	public void setDomainSID(String domainSID)
	{
		this.domainSID = domainSID;
	}

	public String getPrimaryGroupSID()
	{
		return primaryGroupSID;
	}

	/**
	 * Called by native code, login0().
	 * @param primaryGroupSID user's primary group SID
	 */
	public void setPrimaryGroupSID(String primaryGroupSID)
	{
		this.primaryGroupSID = primaryGroupSID;
	}

	public String[] getGroupSIDs()
	{
		return groupSIDs;
	}

	/**
	 * Called by native code, login0().
	 * @param groupSIDs SIDs of other groups, the user belong to.
	 */
	public void setGroupSIDs(String[] groupSIDs)
	{
		this.groupSIDs = groupSIDs;
	}

	public long getImpersonationToken()
	{
		return impersonationToken;
	}
}
