#ifndef _SNF_NET_PLAT_H_
#define _SNF_NET_PLAT_H_

#if defined(_WIN32)

#include <WinSock2.h>
#include <ws2tcpip.h>

constexpr const char *LIBSSL = "ssl.dll";
constexpr const char *LIBCRYPTO = "crypto.dll";


using in_port_t = uint16_t;
using sock_t = SOCKET;

constexpr int SHUTDOWN_READ = SD_RECEIVE;
constexpr int SHUTDOWN_WRITE = SD_SEND;
constexpr int SHUTDOWN_RDWR = SD_BOTH;

constexpr bool connect_in_progress(int e) { return (e == WSAEWOULDBLOCK); }

#if !defined(ETIMEDOUT)
#define WSAETIMEDOUT ETIMEDOUT
#endif

#else // !_WIN32

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <poll.h>

constexpr const char *LIBSSL = "libssl.so";
constexpr const char *LIBCRYPTO = "libcrypto.so";

using sock_t = int;
constexpr sock_t INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;
constexpr int SHUTDOWN_READ = SHUT_RD;
constexpr int SHUTDOWN_WRITE = SHUT_WR;
constexpr int SHUTDOWN_RDWR = SHUT_RDWR;

constexpr bool connect_in_progress(int e) { return (e == EINPROGRESS); }

#endif // _WIN32

#endif // _SNF_NET_PLAT_H_
