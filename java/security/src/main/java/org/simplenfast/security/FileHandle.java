/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.simplenfast.security;

import java.io.FileDescriptor;

/**
 *
 * @author Moji
 */
class FileHandle
{
    private static final sun.misc.JavaIOFileDescriptorAccess FD_ACCESS =
		sun.misc.SharedSecrets.getJavaIOFileDescriptorAccess();	
	private static final String OS_NAME = System.getProperty("os.name").toLowerCase();

	static FileDescriptor getFileDescriptor(long handle)
	{
		FileDescriptor fd = new FileDescriptor();
		if (OS_NAME.contains("windows")) {
			FD_ACCESS.setHandle(fd, handle);
		} else {
			FD_ACCESS.set(fd, (int) handle);
		}

		return fd;
	}

	static long getFileHandle(FileDescriptor fd)
	{
		if (OS_NAME.contains("windows")) {
			return FD_ACCESS.getHandle(fd);
		} else {
			return FD_ACCESS.get(fd);
		}
	}
}
