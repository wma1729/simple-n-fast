# libnet

Libnet is a networking library that makes it simple to to do basic network programming.
- Simple consistent interface.
- Hides platform differences.
- Support for secured communication.
- Dynamically loads OpenSSL/LibreSSL libraries.

Platform | Default SSL Library Name | Override With               | Lookup
-------- | ------------------------ | --------------------------- | ------
Linux    | libssl.so                | Environment Variable LIBSSL | LD_LIBRARY_PATH
Windows  | ssl.dll                  | Environment Variable LIBSSL | Path

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
`snf::net::ssl::pkey` | Encapsulates OpenSSL key (EVP_PKEY).
`snf::net::ssl::x509_certificate` | Encapsulates OpenSSL X509 Certificate (X509).
`snf::net::ssl::x509_crl` | Encapsulates OpenSSL X509 Certificate Revocation List (X509_crl).
`snf::net::ssl::truststore` | Encapsulates OpenSSL X509 trust store (X509_STORE).
`snf::net::ssl::session` | Encapsulates OpenSSL session (SSL_SESSION).
`snf::net::ssl::context` | Encapsulates OpenSSL SSL context (SSL_CTX).
`snf::net::ssl::connection` | Represents secured TLS connection. Manages all aspects of a secured connection.

The commonly thrown exception other than the ones listed above is `snf::net::ssl::exception`. Here is a simple example on how to catch and log the exception:

```C++
try {
    ...
} catch (snf::net::ssl::exception &ex) {
    std::cerr << ex.what() << std::endl;
    for (auto it = ex.begin(); it != ex.end(); ++it)
        std::cerr << *it << std::endl;
}
```

### Prepare SSL context
```C++
// Create context.
snf::net::ssl::context ctx;

// Add private key (from file)
snf::net::ssl::pkey key { key_format, key_file, key_file_password };
ctx.use_private_key(key);

// Add certificate (from file)
snf::net::ssl::x509_certificate cert { cert_format, cert_file };
ctx.use_certificate(cert);

// Check if the private key matches the certificate?
ctx.check_private_key();

// Add trust store
snf::net::ssl::truststore store { cert_chain_file };
snf::net::ssl::x509_crl crl { crl_file};
store.add_crl(crl);
ctx.use_truststore(store);

// Set ciphers to use (default list)
ctx.set_ciphers(); 

// Verify peer
ctx.verify_peer(require_peer_certificate);

// Set certificate chain depth
ctx.limit_certificate_chain_depth(depth);

// If using session ID based session resumption, set session ID context on TLS server.
ctx.set_session_context(session_id_ctx);

// If you intend to use session ticket based session resumption, enable it.
// Connection mode is snf::net::connection_mode : client or server.
ctx.session_ticket(connection_mode, true);
```

### Perform handshake

#### TLS Client
```C++
// Prepare SSL context

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
// Prepare SSL context

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Bind to the port.
sock.bind(AF_INET, port);

// Start listening...
sock.listen(backlog);

// Accept new connection: in a loop or when the socket is ready (select/poll)
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

// Add the host names to be checked
cnxn.check_hosts( { "www.example.com", "example.com" } );

// Perform TLS handshake.
cnxn.handshake(nsock);
```

#### Validation code for internet address
```C++
...

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Add the internet address to be checked
snf::net::internet_address ia { "172.18.0.1" };
cnxn.check_inaddr(ia);

// Perform TLS handshake.
cnxn.handshake(nsock);
```

### TLS session resumption
TLS handshake is expensive. To speed up things, TLS provides session resumption. The idea is to save session state from a previously successful handshake and reusing the state for the next connection. There are two methods available.

### TLS resumption uisng session ID
TLS server maintains the session state in this approach. When the client and server decide to resume a previous session the TLS client sends the session ID of the session to be resumed to the TLS server. The server then checks its session cache for a match. If a match is found, and the server is willing to re-establish the connection, the session state is reused. If a session ID match is not found, the server generates a new session ID, and the TLS client and server perform a full handshake.

#### TLS Client
```C++
// Prepare SSL context

// Create socket.
snf::net::socket sock { AF_INET, snf::net::socket_type::tcp };

// Set socket options as needed.
...

// Establish the connection.
sock.connect(AF_INET, host, port);

// Create secured connection.
snf::net::ssl::connection cnxn { snf::net::connection_mode::client, ctx };

// Assuming that the session state is stored in a file (from last successful handshake)
snf::net::ssl::session sess { session_file };
cnxn->set_session(sess);

// Perform TLS handshake.
cnxn.handshake(sock);

// Get session details after the handshake.
snf::net::ssl::session sess = std::move(cnxn->get_session());
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
// Prepare SSL context

// Set the session ID context.
ctx.set_session_context(session_id_ctx);

// The rest of the code remains the same.

```
