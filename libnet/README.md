# libnet
Libnet is a networking library that makes it simple to to do basic network programming.
- Simple consistent interface.
- Hides platform differences.
- Support for secured communication.
- Dynamically loads OpenSSL/LibreSSL libraries.

Platform | Default SSL Library Name | Override With               | Lookup
-------- | ------------------------ | --------------------------- | ------
Linux    | libssl.so                | Environment Variable LIBSSL | LD_LIBRARY_PATH
Windows  | ssl.dll                  | Environment Variable LIBSSL | PATH

### Library interface
Look at the include files for interface. The code is well documented to give you an idea. For more details, continue reading.

### Initialization & Finalization
```C++
void snf::net::initialize(bool use_ssl);
void snf::net::finalize();
```

### Classes for un-secured communication
The library provides the following classes for basic networking:

Class Name | Purpose
---------- | -------
`snf::net::internet_address` | Encapsulates AF_INET and AF_INET6 internet address.
`snf::net::socket_address`   | Encapsulates socket address: address family, address, and port.
`snf::net::host`             | Host lookup related functionality. There is a `snf::net::hosteq` function that determines if two hosts are equal.
`snf::net::nio`              | Implemented by `snf::net::socket` and `snf::net::ssl::connection` to provide a consistent read/write interfaces.
`snf::net::socket`           | Implements most of the commonly used TCP socket functionality: opening and closing of socket, getting/setting socket options, connect/bind/listen/accept and related operations, etc.

Most of the functions provide timeout feature. The commonly thrown exceptions are:
- `std::invalid_argument`
- `std::runtime_error`
- `std::system_error`

Check source code documentation for details.

### Classes for secured communication
The library provides the following classes for secured networking:

Class Name | Purpose
---------- | -------
`snf::ssl::pkey` | Encapsulates OpenSSL key (EVP_PKEY).
`snf::ssl::x509_certificate` | Encapsulates OpenSSL X509 Certificate (X509).
`snf::ssl::x509_crl` | Encapsulates OpenSSL X509 Certificate Revocation List (X509_crl).
`snf::ssl::truststore` | Encapsulates OpenSSL X509 trust store (X509_STORE).
`snf::ssl::session` | Encapsulates OpenSSL session (SSL_SESSION).
`snf::ssl::context` | Encapsulates OpenSSL SSL context (SSL_CTX).
`snf::net::ssl::connection` | Represents secured TLS connection. Manages all aspects of a secured connection.

The commonly thrown exception other than the ones listed above is `snf::ssl::exception`. Here is a simple example on how to catch and log the exception:

```C++
try {
    ...
} catch (snf::ssl::exception &ex) {
    std::cerr << ex.what() << std::endl;
    for (auto it = ex.begin(); it != ex.end(); ++it)
        std::cerr << *it << std::endl;
}
```

### Prepare SSL context
```C++
// Create context.
snf::ssl::context ctx;

// Add private key (from file).
snf::ssl::pkey key { key_format, key_file, key_file_password };
ctx.use_private_key(key);

// Add certificate (from file).
snf::ssl::x509_certificate cert { cert_format, cert_file };
ctx.use_certificate(cert);

// Check if the private key matches the certificate?
ctx.check_private_key();

// Add trust store.
snf::ssl::truststore store { cert_chain_file };
snf::ssl::x509_crl crl { crl_file};
store.add_crl(crl);
ctx.use_truststore(store);

// Set ciphers to use (default list).
ctx.set_ciphers(); 

// Verify peer.
ctx.verify_peer(require_peer_certificate);

// Set certificate chain depth.
ctx.limit_certificate_chain_depth(depth);
```

### Perform handshake
#### TLS Client
```C++
// Prepare SSL context.

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Establish the connection.
sock.connect(AF_INET, host, port);

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Perform TLS handshake.
cnxn.handshake(sock);
```

#### TLS Server
```C++
// Prepare SSL context.

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Bind to the port.
sock.bind(AF_INET, port);

// Start listening...
sock.listen(backlog);

// Accept new connection: in a loop or when the socket is ready (select/poll).
snf::net::socket nsock = std::move(sock.accept());

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::server, ctx };

// Perform TLS handshake.
cnxn.handshake(nsock);
```

### Host Name (or internet address) Validation
Very basic host name validation is provided. The side (client or server) that wants to perform validation sets the host name(s) or internet address before beginning the handshake. The SSL context must have peer verification set. If any of the host name or the internet address matches any of the host name/internet address in the peer certificate, the verification passes. Otherwise the verification fails and so does the handshake.

#### Validation code for host name(s)
```C++
...

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Add the host names to be checked.
cnxn.check_hosts( { "www.example.com", "example.com" } );

// Perform TLS handshake.
cnxn.handshake(nsock);
```

#### Validation code for internet address
```C++
...

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Add the internet address to be checked.
snf::net::internet_address ia { "172.18.0.1" };
cnxn.check_inaddr(ia);

// Perform TLS handshake.
cnxn.handshake(nsock);
```

### TLS session resumption
TLS handshake is expensive. To speed up things, TLS provides session resumption. The idea is to save session state from a previously successful handshake and reusing the state for the next connection. There are two methods available.

### TLS resumption using session ID
TLS server maintains the session state in this approach. When the client and server decide to resume a previous session the TLS client sends the session ID of the session to be resumed to the TLS server. The server then checks its session cache for a match. If a match is found, and the server is willing to re-establish the connection, the session state is reused. If a session ID match is not found, the server generates a new session ID, and the TLS client and server perform a full handshake.

#### TLS Client
```C++
// Prepare SSL context.

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Establish the connection.
sock.connect(AF_INET, host, port);

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Assuming that the session state is stored in a file (from last successful handshake).
snf::ssl::session sess { session_file };
cnxn->set_session(sess);

// Perform TLS handshake.
cnxn.handshake(sock);

// Get session details after the handshake.
snf::ssl::session sess = std::move(cnxn->get_session());
std::cout << "session id = " << sess.get_id() << std::endl;

// Check if the session is reused?
if (cnxn->is_session_reused())
	std::cout << "SSL session is reused" << std::endl;
else
	std::cout << "SSL session is not reused" << std::endl;

// Persist the session if needed.
sess.to_file(session_file);
```

#### TLS Server
Nothing is needed on the server side other than make sure that session context is set.
```C++
// Prepare SSL context.

// Set the session ID context.
ctx.set_session_context(session_id_ctx);

// The rest of the code remains the same.
```

### TLS session resumption using session ticket
TLS client maintains the session state in this approach. A session ticket is a blob of session state that is encrypted using a key maintained by the TLS server. After a successful handshake, the server sends the ticket to the client. The client saves the ticket. When the connection is re-attempted using the saved session ticket, the client includes the key in the handsheke message. Only the server can decrypt the session ticket and determine if the session can be reused.

#### TLS Client
```C++
// Prepare SSL context.

// Enable ticket based session resumption.
ctx.session_ticket(snf::net::connection_mode::client, true);

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Establish the connection.
sock.connect(AF_INET, host, port);

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Assuming that the session state is stored in a file (from last successful handshake).
snf::ssl::session sess { session_file };
cnxn->set_session(sess);

// Perform TLS handshake.
cnxn.handshake(sock);

// Get session details after the handshake.
snf::ssl::session sess = std::move(cnxn->get_session());
std::cout << "session id = " << sess.get_id() << std::endl;

// Check if the session is reused?
if (cnxn->is_session_reused())
	std::cout << "SSL session is reused" << std::endl;
else
	std::cout << "SSL session is not reused" << std::endl;

// Persist the session if needed.
sess.to_file(session_file);
```

#### TLS Server
Again not much is needed here.
```C++
// Prepare SSL context.

// Set the session ID context.
ctx.set_session_context(session_id_ctx);

// Enable ticket based session resumption.
ctx.session_ticket(snf::net::connection_mode::server, true);

/*
 * By default, the TLS server implementation uses a default key manager. A customized
 * key manager can be deployed at this time. More details can be found in the source
 * code: ctx.h and keymgr.h
 * snf::ssl::context::set_keymgr(custom_keymgr);
 */

// The rest of the code remains the same.
```

### Server Name Indication (SNI)
A TLS server may be running on a host that has multiple DNS host names. Let's say the host names are H1, H2, H3, ... Hn. One possible approach is to have a single certificate with H1 as the subject Common Name and H2, H3, ... Hn as the Subject Alternate Names. Now the TLS client has the responsibility of verifying that the server name it connected to matches one of the names in the certificate. This approach is fine in most cases. But when it becomes difficult (and it happens fairly often in reality) to find all the server names in advance (as the names may change), this approach does not work. It sort of work but is very static.

SNI, a TLS extension, comes to the rescue here. The solution is to have one certificate per host name on the TLS server. All the certificates (in different SSL contexts) are registered with the TLS server. When the client starts the TLS handshake, it passes in the name of the server. The TLS servr looks at the server name and switches to the right certificate (SSL context) transparently.

#### TLS Client
```C++
// Prepare SSL context.

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Establish the connection.
sock.connect(AF_INET, host, port);

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Set the server name that you are connecting to.
cnxn.set_sni(host);

// Check the server name as part of verification.
cnxn.check_hosts( { host } );

// Perform TLS handshake.
cnxn.handshake(sock);

// If the handshake is successful, it is because the certificate is for the host name 'host'.
```

#### TLS server
```C++
// Prepare SSL context.

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Bind to the port.
sock.bind(AF_INET, port);

// Start listening...
sock.listen(backlog);

// Accept new connection: in a loop or when the socket is ready (select/poll).
snf::net::socket nsock = std::move(sock.accept());

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::server, ctx };

// Prepare SSL contexts (one for each host name) and register them with connection.
for (auto c : contexts)
	cnxn.add_context(c);

// Enable SNI (this registers callback that does SSL context switch).
cnxn.enable_sni();

// Perform TLS handshake.
cnxn.handshake(nsock);
```
