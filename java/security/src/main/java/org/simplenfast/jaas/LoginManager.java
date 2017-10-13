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

import java.security.Principal;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.security.auth.Subject;

import javax.security.auth.login.LoginContext;
import javax.security.auth.login.LoginException;

/**
 * Provides an easy to use interface to JAAS Login Modules. 
 * The login entry name in the JAAS configuration files is obtained
 * from the system property <code>simplenfast.jaas.config.entry</code>.
 */
public class LoginManager
{
	private static final Logger LOGGER = Logger.getLogger(LoginManager.class.getName());
	private static LoginManager mgr;
	private final String jaasConfigEntry;

	private LoginManager()
	{
		jaasConfigEntry = System.getProperty("simplenfast.jaas.config.entry");
		if (jaasConfigEntry == null) {
			throw new IllegalArgumentException("System property \"simplenfast.jaas.config.entry\" not set");
		}
	}

	/**
	 * Gets an instance of the LoginManager.
	 * @return login manager instance.
	 */
	public static synchronized LoginManager getInstance()
	{
		if (mgr == null) {
			mgr = new LoginManager();
		}
		return mgr;
	}

	/**
	 * Provides the login mechanism using ConsoleCallbackHandler.
	 * The user will be prompted for the user name and password.
	 * 
	 * @return the login context if successful, null on failure.
	 */
	public LoginContext login()
	{
		final String who = "login";
		LoginContext loginCtx; 

		try {
			loginCtx = new LoginContext(jaasConfigEntry, new ConsoleCallbackHandler());
		} catch (LoginException ex) {
			LOGGER.log(Level.SEVERE,
				String.format("%s: failed to create login context: %s", who, ex.getMessage()), ex);
			loginCtx = null;
		} catch (SecurityException ex) {
			LOGGER.log(Level.SEVERE,
				String.format("%s: security exception while creating login context: %s", who, ex.getMessage()), ex);
			loginCtx = null;
		}

		if (loginCtx != null) {
			try {
				loginCtx.login();
			} catch (LoginException ex) {
				LOGGER.log(Level.SEVERE,
					String.format("%s: failed to login: %s", who, ex.getMessage()), ex);
				loginCtx = null;
			}
		}

		return loginCtx;
	}

	/**
	 * Provides the login mechanism using NoPromptCallbackHandler.
	 * The user name and password are specified as parameters.
	 * 
	 * @param name user name
	 * @param password user password
	 * 
	 * @return the login context if successful, null on failure.
	 */
	public LoginContext login(String name, char [] password)
	{
		final String who = "login";
		LoginContext loginCtx; 

		try {
			loginCtx = new LoginContext(jaasConfigEntry, new NoPromptCallbackHandler(name, password));
		} catch (LoginException ex) {
			LOGGER.log(Level.SEVERE,
				String.format("%s: failed to create login context: %s", who, ex.getMessage()), ex);
			loginCtx = null;
		} catch (SecurityException ex) {
			LOGGER.log(Level.SEVERE,
				String.format("%s: security exception while creatng login context: %s", who, ex.getMessage()), ex);
			loginCtx = null;
		}

		if (loginCtx != null) {
			try {
				loginCtx.login();
			} catch (LoginException ex) {
				LOGGER.log(Level.SEVERE,
					String.format("%s: failed to login: %s", who, ex.getMessage()), ex);
				loginCtx = null;
			}
		}

		return loginCtx;
	}

	/**
	 * Provides the logout mechanism.
	 * 
	 * @param loginCtx the login context obtained from <code>login</code>.
	 */
	public void logout(LoginContext loginCtx)
	{
		final String who = "logout";

		try {
			loginCtx.logout();
		} catch (LoginException ex) {
			LOGGER.log(Level.SEVERE,
				String.format("%s: failed to logout: %s", who, ex.getMessage()), ex);
		}
	}

	/*
	 * A test method.
	 */
	public static void main(String [] args)
	{
		LoginManager loginMgr = LoginManager.getInstance();
		LoginContext lc = loginMgr.login();
		if (lc != null) {
			Subject subject = lc.getSubject();
			Set<Principal> principals = subject.getPrincipals();
			principals.forEach((p) -> {
				System.out.println(p.getName());
			});
			loginMgr.logout(lc);
		} else {
			System.err.println("Authentication error");
		}
	}
}
