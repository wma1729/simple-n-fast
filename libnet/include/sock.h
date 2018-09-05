#ifndef _SNF_SOCK_H_
#define _SNF_SOCK_H_

#include "net.h"

namespace snf {
namespace net {

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
	const socket_address &local_address();
	const socket_address &peer_address();
	void close();
	void shutdown(int);
};

inline std::ostream &
operator<<(std::ostream &os, socket::linger_type lt)
{
	if (lt == socket::linger_type::dflt)
		os << "default";
	else if (lt == socket::linger_type::none)
		os << "none";
	else // if (lt == socket::linger_type::timed)
		os << "timed";

	return os;
}

} // namespace net
} // namespace snf

#endif // _SNF_SOCK_H_
