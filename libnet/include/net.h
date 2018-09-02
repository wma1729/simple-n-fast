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

bool initialize();
void finalize();

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

class socket
{
private:
	sock_t          m_sock;
	socket_type     m_type;
	socket_address  *m_local = nullptr;
	socket_address  *m_peer = nullptr;

	const char *optstr(int, int);
	void getopt(int, int, void *, int *);
	void setopt(int, int, void *, int);

public:
	enum class linger_type
	{
		dflt,	// default linger behaviour
		none,	// no lingering
		timed	// linger for the specified time
	};

	socket(sock_t);
	socket(int, socket_type);
	~socket();

	socket_type get_type() { return m_type; }
	bool keepalive();
	void keepalive(bool);
	bool reuseaddr();
	void reuseaddr(bool);
	linger_type linger(int *to = nullptr);
	void linger(linger_type, int to = 60);
	int rcvbuf();
	void rcvbuf(int);
	int sndbuf();
	void sndbuf(int);
	int64_t rcvtimeout();
	void rcvtimeout(int64_t);
	int64_t sndtimeout();
	void sndtimeout(int64_t);
	bool tcpnodelay();
	void tcpnodelay(bool);
	void blocking(bool);
	void close();
};

inline std::ostream &
operator<<(std::ostream &os, socket::linger_type lt)
{
	if (lt == socket::linger_type::dflt)
		os << "default";
	else if (lt == socket::linger_type::none)
		os << "none";
	else if (lt == socket::linger_type::timed)
		os << "timed";
	else
		os << "unknown";

	return os;
}

} // namespace net
} // namespace snf

#endif // _SNF_NET_H_
