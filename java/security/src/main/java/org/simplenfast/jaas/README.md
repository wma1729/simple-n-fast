# JAAS Login Modules

## NT Login Module

Provides NT user authentication using Win32 API. The main class is `org.simplenfast.jaas.NTLoginModule`. The JAAS configuration file looks like:
```
<login_entry_name> {
	org.simplenfast.jaas.NTLoginModule required
		debug = true;
};
```

This class relies on `ntuser.dll`. System property `simplenfast.jaas.libpath` is used to specify the full path of the library.

## PAM Login Module
Provides Unix user authentication using PAM. The main class is `org.simplenfast.jaas.PAMLoginModule`. Here is a sample JAAS configuration file:
```
<login_entry_name> {
	org.simplenfast.jaas.PAMLoginModule required
		service = ssh
		debug = true;
}
```
The valid service name are: ssh, login, passwd, etc. Check PAM documentation for a valid list of services.

This class relies on `pamuser.dll`. System property `simplenfast.jaas.libpath` is used to specify the full path of the library.

As always, system property `java.security.auth.login.config` is used to specify the JAAS configuration file path.

There is a helper class, `org.simplenfast.jaas.LoginManager`, provided for easy access to the login modules.
