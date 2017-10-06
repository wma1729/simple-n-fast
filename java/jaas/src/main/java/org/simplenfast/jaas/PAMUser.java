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

class PAMUser
{
	private native long login0(String serviceName, String userName, char [] password)
						throws NativeException;
	private native int logout0(long context);

	private final String serviceName;
	private final String userName;
	private final char [] password;
	private long context;
	private long UID;
	private long GID;
	private long [] supplementaryGIDs;

	PAMUser(String serviceName, String userName, char [] password)
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
		System.loadLibrary("pam");
	}

	synchronized boolean login()
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

	synchronized boolean logout()
	{
		if (context != 0) {
			if (logout0(context) != 0) {
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

	void setUID(long UID)
	{
		this.UID = UID;
	}

	long getGID()
	{
		return GID;
	}

	void setGID(long GID)
	{
		this.GID = GID;
	}

	long[] getSupplementaryGIDs()
	{
		return supplementaryGIDs;
	}

	void setSupplementaryGIDs(long[] supplementaryGIDs)
	{
		this.supplementaryGIDs = supplementaryGIDs;
	}
}
