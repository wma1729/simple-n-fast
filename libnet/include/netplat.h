#ifndef _SNF_NET_PLAT_H_
#define _SNF_NET_PLAT_H_

#if defined(_WIN32)

#include <WinSock2.h>
#include <ws2tcpip.h>

using in_port_t = uint16_t;

#else // !_WIN32

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#endif // _WIN32

#endif // _SNF_NET_PLAT_H_
