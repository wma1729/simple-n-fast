#ifndef _SNF_NET_PLAT_H_
#define _SNF_NET_PLAT_H_

#if defined(_WIN32)

#include <WinSock2.h>
#include <ws2tcpip.h>

typedef uint16_t in_port_t;

#else // !_WIN32

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

inline int
net_error(void)
{
	return errno;
}

inline void
net_error(int e)
{
	errno = e;
}

#endif // _WIN32

#endif // _SNF_NET_PLAT_H_
