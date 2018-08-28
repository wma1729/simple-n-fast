#ifndef _SNF_NET_PLAT_H_
#define _SNF_NET_PLAT_H_

#if defined(_WIN32)

#include <WinSock2.h>
#include <ws2tcpip.h>

using in_port_t = uint16_t;
using sock_t = SOCKET;

#else // !_WIN32

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>

using sock_t = int;
constexpr sock_t INVALID_SOCKET = -1;
constexpr int SOCKET_ERROR = -1;

#endif // _WIN32

#endif // _SNF_NET_PLAT_H_
