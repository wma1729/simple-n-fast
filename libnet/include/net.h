#ifndef _SNF_NET_H_
#define _SNF_NET_H_

#include "common.h"
#include "netplat.h"
#include "sa.h"
#include <stdexcept>

namespace snf {
namespace net {

inline int
error(void)
{
#if defined(_WIN32)
	return ::WSAGetLastError();
#else
	return errno;
#endif
}

inline void
error(int e)
{
#if defined(_WIN32)
	::WSASetLastError(e);
#else
	errno = e;
#endif
}

void initialize(bool use_ssl = false);
void finalize();
unsigned long openssl_version(std::string &);

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

constexpr int POLL_WAIT_FOREVER = -1;
constexpr int POLL_WAIT_NONE = 0;

int poll(std::vector<pollfd> &, int, int *);

} // namespace net
} // namespace snf

#endif // _SNF_NET_H_
