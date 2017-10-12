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

import java.io.IOException;

import javax.security.auth.callback.Callback;
import javax.security.auth.callback.CallbackHandler;
import javax.security.auth.callback.NameCallback;
import javax.security.auth.callback.PasswordCallback;
import javax.security.auth.callback.UnsupportedCallbackException;

/**
 * Reads the user credentials from the console and pass them to
 * the JAAS login module.
 */
public class ConsoleCallbackHandler implements CallbackHandler
{
	@Override
	public void handle(Callback[] callbacks)
		throws IOException, UnsupportedCallbackException
	{
		for (int i = 0; i < callbacks.length; ++i) {
			if (callbacks[i] instanceof NameCallback) {
				
				NameCallback nc = (NameCallback) callbacks[i];
				System.out.print(nc.getPrompt());
				System.out.flush();
				nc.setName(System.console().readLine());

			} else if (callbacks[i] instanceof PasswordCallback) {

				PasswordCallback pc = (PasswordCallback) callbacks[i];
				System.out.print(pc.getPrompt());
				System.out.flush();
				pc.setPassword(System.console().readPassword());

			} else {
				throw new UnsupportedCallbackException(callbacks[i], "callback not handled");
			}
		}
	}
}
