/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.simplenfast.security;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;
import javax.security.auth.login.LoginException;
import org.simplenfast.security.JniUser;
import org.simplenfast.security.NTUser;
import org.simplenfast.security.PAMUser;

/**
 *
 */
public class Process
{
	private static final String OS_NAME = System.getProperty("os.name").toLowerCase();

	private final String binary;
	private final List<String> arguments = new ArrayList<>();
	private String directory = null;
	private final long [] stdFd = new long[3];
	private JniUser subject = null;
	private long processCtx = -1;

	public Process(String binary)
	{
		this.binary = binary;
		this.stdFd[0] = -1;
		this.stdFd[1] = -1;
		this.stdFd[2] = -1;
	}

	public Process(String binary, String ... args)
	{
		this.binary = binary;
		this.arguments.addAll(Arrays.asList(args));
		this.stdFd[0] = -1;
		this.stdFd[1] = -1;
		this.stdFd[2] = -1;
	}

	public String getDirectory()
	{
		return directory;
	}

	public void setDirectory(String directory)
	{
		this.directory = directory;
	}

	public OutputStream getInputStream(FileDescriptor fd)
	{
		if (stdFd[0] != -1)
			return new FileOutputStream(FileHandle.getFileDescriptor(stdFd[0]));
		return null;
	}

	public InputStream getOutputStream(FileDescriptor fd)
	{
		if (stdFd[1] != -1)
			return new FileInputStream(FileHandle.getFileDescriptor(stdFd[1]));
		return null;
	}

	public InputStream getErrorStream(FileDescriptor fd)
	{
		if (stdFd[2] != -1)
			return new FileInputStream(FileHandle.getFileDescriptor(stdFd[2]));
		return null;
	}

	public boolean login(String user, char [] password)
		throws LoginException
	{
		if (OS_NAME.contains("windows")) {
			subject = new NTUser(user, password);
		} else {
			subject = new PAMUser("login", user, password);
		}

		return subject.login();
	}

	public boolean start()
		throws IOException
	{
		if (subject != null) {
			processCtx = subject.execute(binary, arguments, directory, stdFd);
		}

		return false;
	}

	public int getExitCode(long duration, TimeUnit unit)
		throws InterruptedException
	{
		int ec = -1;
		if ((subject != null) && (processCtx != -1)) {
			ec = subject.getExitCode(processCtx, duration, unit);
		}

		return ec;
	}

	public boolean logout()
	{
		if (subject != null) {
			return subject.logout();
		}
		return false;
	}
}
