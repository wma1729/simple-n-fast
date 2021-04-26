#ifndef _SNF_NET_H_
#define _SNF_NET_H_

#include "common.h"
#include "netplat.h"
#include "sa.h"
#include <stdexcept>

namespace snf {
namespace net {

/* Get system error */
inline int
error(void)
{
#if defined(_WIN32)
	return ::WSAGetLastError();
#else
	return errno;
#endif
}

/* Set system error */
inline void
error(int e)
{
#if defined(_WIN32)
	::WSASetLastError(e);
#else
	errno = e;
#endif
}

/*
 * Initialize the networking library.
 * If use_ssl flag is set, the SSL library is
 * initialized as well. This function can be
 * called multiple times.
 */
void initialize();

/* Finalizes (cleans up) the networking library. */
void finalize();

/*
 * Openssl library is loaded dynamically.
 * It may be desirable to get the version of
 * openssl library loaded. This function can
 * be used to get the openssl version.
 */
unsigned long openssl_version(std::string &);

/*
 * Swap bytes of an integral number.
 */
template<typename T>
T swap(T x)
{
	uint8_t *p = reinterpret_cast<uint8_t *>(&x);
	int i = 0;
	int j = sizeof(T) - 1;

	while (i < j) {
		uint8_t c = p[i];
		p[i++] = p[j];
		p[j--] = c;
	}

	return x;
}

/*
 * Converts the integral data type from host to
 * network byte order.
 */
template<typename T>
T hton(T b)
{
	if (sizeof(T) == 2) {
		return snf::narrow_cast<T>(htons(b));
	} else if (sizeof(T) == 4) {
		return snf::narrow_cast<T>(htonl(b));
	} else if (sizeof(T) == 8) {
		if (snf::is_big_endian())
			return b;
		return swap(b);
	}
	throw std::invalid_argument("invalid data type");
}

/*
 * Converts the integral data type from network to
 * host byte order.
 */
template<typename T>
T ntoh(T b)
{
	if (sizeof(T) == 2) {
		return snf::narrow_cast<T>(ntohs(b));
	} else if (sizeof(T) == 4) {
		return snf::narrow_cast<T>(ntohl(b));
	} else if (sizeof(T) == 8) {
		if (snf::is_big_endian())
			return b;
		return swap(b);
	}
	throw std::invalid_argument("invalid data type");
}

int map_system_error(int, int);

/*
 * Connection mode
 * client - side calling connect()
 * server - side calling accept()
 */
enum class connection_mode { client, server };

constexpr int POLL_WAIT_FOREVER = -1;
constexpr int POLL_WAIT_NONE = 0;

int poll(std::vector<pollfd> &, int, int *oserr = 0);

} // namespace net
} // namespace snf

#endif // _SNF_NET_H_
