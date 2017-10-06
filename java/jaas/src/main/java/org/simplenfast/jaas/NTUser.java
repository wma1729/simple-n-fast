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

import javax.security.auth.login.LoginException;

public class NTUser
{
	private native long login0(NTUser ntUser, String domainName, String userName, char [] password)
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
		this.domainSID = null;
		this.primaryGroupSID = null;
		this.groupSIDs = null;
		this.impersonationToken = 0;

		String libPath = System.getProperty("simplenfast.jaas.libpath");
		if (libPath != null) {
			System.load(libPath);
		}
	}

	public synchronized boolean login()
		throws LoginException
	{
		if (impersonationToken != 0) {
			return true;
		}

		try {
			impersonationToken = login0(this, domainName, userName, password);
		} catch (NativeException ex) {
			throw NativeUtil.NativeToLoginException(ex);
		}

		return (impersonationToken != 0);
	}

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

	public void setUserSID(String userSID)
	{
		this.userSID = userSID;
	}

	public String getDomainSID()
	{
		return domainSID;
	}

	public void setDomainSID(String domainSID)
	{
		this.domainSID = domainSID;
	}

	public String getPrimaryGroupSID()
	{
		return primaryGroupSID;
	}

	public void setPrimaryGroupSID(String primaryGroupSID)
	{
		this.primaryGroupSID = primaryGroupSID;
	}

	public String[] getGroupSIDs()
	{
		return groupSIDs;
	}

	public void setGroupSIDs(String[] groupSIDs)
	{
		this.groupSIDs = groupSIDs;
	}

	public long getImpersonationToken()
	{
		return impersonationToken;
	}
}
