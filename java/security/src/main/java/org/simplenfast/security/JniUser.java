/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package org.simplenfast.security;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.TimeUnit;
import javax.security.auth.login.LoginException;

/**
 * JniUser: could be NTUser or PAMUser
 */
interface JniUser
{
	boolean login() throws LoginException;
	long execute(String binary, List<String> args, String dir, long [] handles) throws IOException;
	int getExitCode(long pid, long timeout, TimeUnit unit) throws InterruptedException;
	boolean logout();
}
