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
import javax.security.auth.login.LoginException;

/**
 * Exception thrown by the native code.
 * 
 * This is a generic exception thrown by the native code using JNI.
 */
public class NativeException extends Throwable
{
	private final int errorCode;
	private final String errorString;

	/**
	 * Initialize the NativeException
	 * 
	 * Called from the native code to create and throw the exception.
	 * 
	 * @param message Exception message. 
	 */
	public NativeException(String message)
	{
		this(message, -1, null);
	}
	/**
	 * Initialize the NativeException.
	 * 
	 * Called from the native code to create and throw the exception.
	 * 
	 * @param message Exception message.
	 * @param errorCode Native error code.
	 * @param errorString Description of the native error code.
	 */
	public NativeException(String message, int errorCode, String errorString)	
	{
		super(message);
		this.errorCode = errorCode;
		this.errorString = errorString;
	}

	@Override
	public String getMessage()
	{
		StringBuilder strBldr = new StringBuilder(super.getMessage());

		if (getErrorString() != null) {
			strBldr.append(": ");
			strBldr.append(getErrorString());
			if (getErrorCode() != -1) {
				strBldr.append(" (");
				strBldr.append(getErrorCode());
				strBldr.append(")");
			}
		} else if (getErrorCode() != -1) {
				strBldr.append(": ");
				strBldr.append(getErrorCode());
		}

		return strBldr.toString();
	}

	/**
	 * Gets the native error code.
	 * 
	 * @return the native error code, generally OS specific.
	 */
	public int getErrorCode()
	{
		return errorCode;
	}

	/**
	 * Gets the detailed error string corresponding to the error code.
	 * 
	 * @return the error string.
	 */
	public String getErrorString()
	{
		return errorString;
	}

	/**
	 * Update the stack trace.
	 * 
	 * Adds the exception generation location in the native code. Called
	 * by the native code.
	 * 
	 * @param methodName Method that threw the exception.
	 * @param fileName File name where the exception was thrown.
	 * @param lineNumber  Line number where the exception was thrown.
	 */
	public void addToStackTrace(String methodName, String fileName, int lineNumber)
	{
		StackTraceElement [] curStk = getStackTrace();
		StackTraceElement [] newStk = new StackTraceElement[curStk.length + 1];
		System.arraycopy(curStk, 0, newStk, 1, curStk.length);
		newStk[0] = new StackTraceElement("Native", methodName, fileName, lineNumber);
		setStackTrace(newStk);
	}

	/**
	 * Returns the corresponding Java LoginException
	 * 
	 * @return standard Java LoginException.
	 */
	public LoginException toLoginException()
	{
		LoginException loginEx = new LoginException(getMessage());
		loginEx.setStackTrace(getStackTrace());
		return loginEx;
	}

	/**
	 * Returns the corresponding Java IOException
	 * 
	 * @return standard Java IOException.
	 */
	public IOException toIOException()
	{
		IOException ioEx = new IOException(getMessage());
		ioEx.setStackTrace(getStackTrace());
		return ioEx;
	}

	/**
	 * Returns the corresponding Java InterruptedException
	 * 
	 * @return standard Java InterruptedException.
	 */
	public InterruptedException toInterruptedException()
	{
		InterruptedException intrEx = new InterruptedException(getMessage());
		intrEx.setStackTrace(getStackTrace());
		return intrEx;
	}
}
