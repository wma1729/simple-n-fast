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

import com.sun.security.auth.UnixNumericGroupPrincipal;
import com.sun.security.auth.UnixNumericUserPrincipal;
import com.sun.security.auth.UnixPrincipal;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
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
 * PAM Login Module
 * 
 * The login/logout facility is provided using libpam interface.
 * 
 * The class relies on the system property <code>simplenfast.jaas.libpath</code>. The
 * value should be set to the full path of the library, <code>pamuser.so</code>.
 * 
 * The JAAS configuration file should be specified using system property
 * <code>java.security.auth.login.config</code>. The configuration file should look
 * like:
 * <code>
 * LoginEntryName {
 *     org.simplenfast.jaas.PAMLoginModule required
 *         debug = true
 *         service = PamServiceName
 * };
 * </code>
 */
public class PamLoginModule implements LoginModule
{
	private static final Logger LOGGER = 
	    Logger.getLogger(PamLoginModule.class.getName());
	private static final String DEBUG_OPTION = "debug";
	private static final String SERVICE_OPTION = "service";

	// underlying authentication
	private PAMUser pamUser;

	private Subject subject;
	private CallbackHandler cbHandler;
	private String username;
	private char [] password;

	// configurable option
	private String service;
	private boolean debug = false;

	// the authentication status
	private boolean authSucceeded = false;
	private boolean commitSucceeded = false;

	// Underlying system info
	private UnixPrincipal userPrincipal;
	private UnixNumericUserPrincipal UIDPrincipal;
	private UnixNumericGroupPrincipal GIDPrincipal;
	private final Set<UnixNumericGroupPrincipal> supplementaryGroups =
	    new HashSet<>();

	/*
	 * Cleans up and resets everything. 
	 */
	private void cleanup()
	{
		supplementaryGroups.clear();
		GIDPrincipal = null;
		UIDPrincipal = null;
		userPrincipal = null;
		commitSucceeded = false;
		authSucceeded = false;
		debug = false;
		service = null;
		password = null;
		username = null;
		cbHandler = null;
		subject = null;
	}

	@Override
	public void initialize(Subject subject,
			       CallbackHandler cbHandler,
			       Map<String, ?> sharedState,
			       Map<String, ?> options)
	{
		this.subject = subject;
		this.cbHandler = cbHandler;

		// initialize any configured options
		if (options != null) {
			if (options.containsKey(DEBUG_OPTION)) {
				debug = "true".equalsIgnoreCase(
				    (String) options.get(DEBUG_OPTION));
			}

			if (options.containsKey(SERVICE_OPTION)) {
				service = (String) options.get(SERVICE_OPTION);
			} else {
				service = "login";
			}
		}
	}

	@Override
	public boolean login() throws LoginException
	{
		final String who = "login";

		// Make sure service name is set
		if (service == null) {
			throw new LoginException(
				"PAM service name is not defined");
		}

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

			this.username = nameCallback.getName();
			this.password = passwordCallback.getPassword();
			passwordCallback.clearPassword();
		} catch (IOException | UnsupportedCallbackException ex) {
			LoginException le = new LoginException("cannot fetch user name and password");
			le.initCause(ex);
			throw le;
		}

		// Authenticate
		this.pamUser = new PAMUser(service, username, password);
		pamUser.login();
		Arrays.fill(password, ' ');

		if (debug) {
			LOGGER.fine(String.format("%s: user name = %s", who, pamUser.getUserName()));
		}
		this.userPrincipal = new UnixPrincipal(pamUser.getUserName());

		long uid = pamUser.getUID();
		if (uid < 0) {
			throw new LoginException("cannot get UID");
		}
		
		if (debug) {
			LOGGER.fine(String.format("%s: UID = %d", who, uid));
		}
		this.UIDPrincipal = new UnixNumericUserPrincipal(uid);

		long gid = pamUser.getGID();
		if (gid < 0) {
			throw new LoginException("cannot get primary GID");
		}
		
		if (debug) {
			LOGGER.fine(String.format("%s: GID = %d", who, gid));
		}
		this.GIDPrincipal = new UnixNumericGroupPrincipal(gid, true);

		long [] gids = pamUser.getSupplementaryGIDs();
		if ((gids != null) && (gids.length > 0)) {
			for (long id : gids) {
				if (id != gid) {
					if (debug) {
						LOGGER.fine(String.format("%s: supplementary GID = %d", who, gid));
					}
					UnixNumericGroupPrincipal ngp = new UnixNumericGroupPrincipal(id, false);
					this.supplementaryGroups.add(ngp);
				}
			}
		}

		this.authSucceeded = true;
		return authSucceeded;
	}

	@Override
	public boolean commit() throws LoginException
	{
		final String who = "commit";

		if (authSucceeded == false) {
			if (debug) {
				LOGGER.severe(String.format(
					"%s: did not add any Principals to Subject as authentication failed", who));
			}
			return false;
		}

		if (subject.isReadOnly()) {
			throw new LoginException("Subject is read-only");
		}

		if (!subject.getPrincipals().contains(userPrincipal))
			subject.getPrincipals().add(userPrincipal);
		if (!subject.getPrincipals().contains(UIDPrincipal))
			subject.getPrincipals().add(UIDPrincipal);
		if (!subject.getPrincipals().contains(GIDPrincipal))
			subject.getPrincipals().add(GIDPrincipal);
		for (UnixNumericGroupPrincipal ngp : supplementaryGroups) {
			if (!subject.getPrincipals().contains(ngp))
				subject.getPrincipals().add(ngp);
		}

		commitSucceeded = true;
		return true;
	}

	@Override
	public boolean abort() throws LoginException
	{
		final String who = "abort";

		if (debug) {
			LOGGER.severe(String.format("%s: aborted authentication attempt", who));
		}

		if (authSucceeded == false) {
			return false;
		} else {
			if (commitSucceeded) {
				logout();
			} else {
				cleanup();
			}
			return true;
		}
	}

	@Override
	public boolean logout() throws LoginException
	{
		if (subject.isReadOnly()) {
			throw new LoginException("Subject is read-only");
		}

		if (pamUser.logout()) {
			subject.getPrincipals().remove(userPrincipal);
			subject.getPrincipals().remove(UIDPrincipal);
			subject.getPrincipals().remove(GIDPrincipal);
			for (UnixNumericGroupPrincipal ngp : supplementaryGroups) {
				subject.getPrincipals().remove(ngp);
			}

			cleanup();
			pamUser = null;
			return true;
		}

		return false;
	}
}
