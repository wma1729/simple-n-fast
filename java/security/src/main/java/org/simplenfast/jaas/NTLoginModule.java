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

import com.sun.security.auth.NTUserPrincipal;
import com.sun.security.auth.NTSidUserPrincipal;
import com.sun.security.auth.NTDomainPrincipal;
import com.sun.security.auth.NTSidDomainPrincipal;
import com.sun.security.auth.NTSidPrimaryGroupPrincipal;
import com.sun.security.auth.NTSidGroupPrincipal;
import com.sun.security.auth.NTNumericCredential;

import java.io.IOException;
import java.security.Principal;
import java.util.Arrays;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;

import javax.security.auth.Subject;
import javax.security.auth.callback.Callback;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.callback.NameCallback;
import javax.security.auth.callback.PasswordCallback;
import javax.security.auth.callback.UnsupportedCallbackException;
import javax.security.auth.login.LoginException;
import javax.security.auth.spi.LoginModule;

/**
 * NT Login Module
 * 
 * The login/logout facility is provided using Win32 APIs.
 * {@link #login} is implemented using <code>LogonUser/ImpersonateLoggedOnUser</code>.
 * {@link #logout} is implemented using <code>RevertToSelf/CloseHandle</code>.
 * 
 * The class relies on the system property <code>simplenfast.jaas.libpath</code>. The
 * value should be set to the full path of the library, <code>ntuser.dll</code>.
 * 
 * The JAAS configuration file should be specified using system property
 * <code>java.security.auth.login.config</code>. The configuration file should look
 * like:
 * <code>
 * LoginEntryName {
 *     org.simplenfast.jaas.NTLoginModule required
 *         debug = true;
 * };
 * </code>
 */
public class NTLoginModule implements LoginModule
{
	private static final Logger LOGGER = Logger.getLogger(NTLoginModule.class.getName());
	private static final String DEBUG_OPTION = "debug";
	private NTUser ntUser;

	// initial state
	private Subject subject;
	private CallbackHandler cbHandler;

	// configurable option
	private boolean debug = false;

	// the authentication status
	private boolean authSucceeded = false;
	private boolean commitSucceeded = false;

	private String userName;
	private char [] password;

	private NTUserPrincipal userPrincipal;
	private NTSidUserPrincipal userSID;
	private NTDomainPrincipal userDomain;
	private NTSidDomainPrincipal domainSID;
	private NTSidPrimaryGroupPrincipal primaryGroup;
	private NTSidGroupPrincipal groups[];
	private NTNumericCredential iToken;

	@Override
	public void initialize(Subject subject, CallbackHandler callbackHandler,
						   Map<String,?> sharedState,
						   Map<String,?> options)
	{
		this.subject = subject;
		this.cbHandler = callbackHandler;

		// initialize any configured options
		if (options != null) {
			if (options.containsKey(DEBUG_OPTION)) {
				debug = "true".equalsIgnoreCase(
				    (String) options.get(DEBUG_OPTION));
			}
		}
	}

	@Override
	public boolean login() throws LoginException
	{
		final String who = "login";
		authSucceeded = false; // Indicate not yet successful

		// Make sure callbackHandle is specified
		if (cbHandler == null) {
			throw new LoginException(
				"No CallbackHandler specified to gather authentication information");
		}

		// Get the user name and password
		try {
			NameCallback nameCallback = new NameCallback("User: ");
			PasswordCallback passwordCallback = new PasswordCallback("Password: ", false);

			Callback[] callbacks = new Callback[2];
			callbacks[0] = nameCallback;
			callbacks[1] = passwordCallback;

			cbHandler.handle(callbacks);

			this.userName = nameCallback.getName();
			this.password = passwordCallback.getPassword();
			passwordCallback.clearPassword();
		} catch (IOException | UnsupportedCallbackException ex) {
			LoginException le = new LoginException("cannot fetch user name and password");
			le.initCause(ex);
			throw le;
		}

		ntUser = new NTUser(userName, password);
		ntUser.login();
		Arrays.fill(password, ' ');

		if (debug) {
			LOGGER.fine("succeeded in importing login info");
		}

		String str = ntUser.getUserName();
		if (str == null) {
			throw new LoginException("cannot get user name");
		}
		
		if (debug) {
			LOGGER.fine(String.format("%s: user name = %s", who, str));
		}
		this.userPrincipal = new NTUserPrincipal(str);

		str = ntUser.getUserSID();
		if (str == null) {
			throw new LoginException("cannot get user SID");
		}
		
		if (debug) {
			LOGGER.fine(String.format("%s: user SID = %s", who, str));
		}
	   	this.userSID = new NTSidUserPrincipal(str);

		str = ntUser.getPrimaryGroupSID();
		if (str == null) {
			throw new LoginException("cannot get user primary group SID");
		}
		
		if (debug) {
			LOGGER.fine(String.format("%s: user primary group SID = %s", who, str));
		}
		this.primaryGroup = new NTSidPrimaryGroupPrincipal(str);

		str = ntUser.getDomainName();
		if (str != null) {
			if (debug) {
				LOGGER.fine(String.format("%s: user domain name = %s", who, str));
			}
			this.userDomain = new NTDomainPrincipal(str);
		}

		str = ntUser.getDomainSID();
		if (str != null) {
			if (debug) {
				LOGGER.fine(String.format("%s: user domain SID = %s", who, str));
			}
			this.domainSID = new NTSidDomainPrincipal(str);
		}

		String [] groupSIDs = ntUser.getGroupSIDs();
		if ((groupSIDs != null) && (groupSIDs.length > 0)) {
			groups = new NTSidGroupPrincipal[groupSIDs.length];
			for (int i = 0; i < groupSIDs.length; i++) {
				groups[i] = new NTSidGroupPrincipal(groupSIDs[i]);
				if (debug) {
					LOGGER.fine(String.format("%s: user group SID = %s", who, groupSIDs[i]));
				}
			}
		}

		long token = ntUser.getImpersonationToken();
		if (token == 0) {
			throw new LoginException("cannot get impersonation token");
		}
		this.iToken = new NTNumericCredential(token);

		authSucceeded = true;
		return authSucceeded;
	}

	@Override
	public boolean commit() throws LoginException
	{
		final String who = "commit";

		if (authSucceeded == false) {
			if (debug) {
				LOGGER.severe(
					String.format( "%s: did not add any Principals to Subject as authentication failed", who));
			}
			return false;
		}

		if (subject.isReadOnly()) {
			throw new LoginException("Subject is read-only");
		}
		Set<Principal> principals = subject.getPrincipals();

		// we must have a userPrincipal - everything else is optional
		if (!principals.contains(userPrincipal)) {
			principals.add(userPrincipal);
		}

		if (userSID != null && !principals.contains(userSID)) {
			principals.add(userSID);
		}

		if (userDomain != null && !principals.contains(userDomain)) {
			principals.add(userDomain);
		}

		if (domainSID != null && !principals.contains(domainSID)) {
			principals.add(domainSID);
		}

		if (primaryGroup != null && !principals.contains(primaryGroup)) {
			principals.add(primaryGroup);
		}

		for (int i = 0; groups != null && i < groups.length; i++) {
			if (!principals.contains(groups[i])) {
				principals.add(groups[i]);
			}
		}

		Set<Object> pubCreds = subject.getPublicCredentials();
		if (iToken != null && !pubCreds.contains(iToken)) {
			pubCreds.add(iToken);
		}

		commitSucceeded = true;
		return true;
	}

	@Override
	public boolean abort() throws LoginException
	{
		final String who = "abort";

		LOGGER.fine(String.format("%s: aborted authentication attempt", who));

		if (authSucceeded == false) {
			return false;
		} else {
			if (commitSucceeded) {
				logout();
			} else {
				ntUser = null;
				userPrincipal = null;
				userSID = null;
				userDomain = null;
				domainSID = null;
				primaryGroup = null;
				groups = null;
				iToken = null;
				authSucceeded = false;
			}
		}
		return authSucceeded;
	}

	@Override
	public boolean logout() throws LoginException
	{
		if (subject.isReadOnly()) {
			throw new LoginException("Subject is read-only");
		}

		if (ntUser.logout()) {
			Set<Principal> principals = subject.getPrincipals();
			if (principals.contains(userPrincipal)) {
				principals.remove(userPrincipal);
			}
			if (principals.contains(userSID)) {
				principals.remove(userSID);
			}
			if (principals.contains(userDomain)) {
				principals.remove(userDomain);
			}
			if (principals.contains(domainSID)) {
				principals.remove(domainSID);
			}
			if (principals.contains(primaryGroup)) {
				principals.remove(primaryGroup);
			}
			for (int i = 0; groups != null && i < groups.length; i++) {
				if (principals.contains(groups[i])) {
					principals.remove(groups[i]);
				}
			}

			Set<Object> pubCreds = subject.getPublicCredentials();
			if (pubCreds.contains(iToken)) {
				pubCreds.remove(iToken);
			}

			authSucceeded = false;
			commitSucceeded = false;
			userPrincipal = null;
			userDomain = null;
			userSID = null;
			domainSID = null;
			groups = null;
			primaryGroup = null;
			iToken = null;
			ntUser = null;

			return true;
		}

		return false;
	}
}
