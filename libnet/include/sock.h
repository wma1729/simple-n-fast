#ifndef _SNF_SOCK_H_
#define _SNF_SOCK_H_

#include "net.h"
#include "nio.h"
#include <array>

namespace snf {
namespace net {

/*
 * Manages all aspects of socket.
 * - There is no copy constructor or copy operator.
 * - There are move constructor and move operator available though.
 * - A type operator is provided to get the raw socket.
 */
class socket : public nio
{
private:
	sock_t          m_sock;
	socket_type     m_type;
	socket_address  *m_local = nullptr;
	socket_address  *m_peer = nullptr;
	bool            m_skip_close = false;

#if defined(_WIN32)
	bool            m_blocking = true;
	int64_t         m_rcvtimeo = 0L;
	int64_t         m_sndtimeo = 0L;
#endif

	const char *optstr(int, int);
	void getopt(int, int, void *, int *);
	void setopt(int, int, void *, int);

protected:
	socket(sock_t, const sockaddr_storage &, socklen_t);

public:
	enum class linger_type
	{
		dflt,	// default linger behaviour
		none,	// no lingering
		timed	// linger for the specified time
	};

	static std::array<socket, 2> socketpair();

	socket(int, socket_type);
	socket(sock_t, bool skip_close = false);
	socket(const socket &) = delete;
	socket(socket &&);

	virtual ~socket();

	const socket &operator=(const socket &) = delete;
	socket &operator=(socket &&);

	operator sock_t () const { return m_sock; }

	socket_type get_type() const { return m_type; }
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
	int error();
	bool tcpnodelay();
	void tcpnodelay(bool);
	bool blocking();
	void blocking(bool);
	std::string dump_options();
	const socket_address &local_address();
	const socket_address &peer_address();
	void connect(int, const std::string &, in_port_t, int to = POLL_WAIT_FOREVER); 
	void connect(const internet_address &, in_port_t, int to = POLL_WAIT_FOREVER);
	void connect(const socket_address &, int to = -1);
	void bind(int, in_port_t);
	void bind(const internet_address &, in_port_t);
	void bind(const socket_address &);
	void listen(int);
	socket accept();
	bool is_readable(int to = POLL_WAIT_FOREVER, int *oserr = 0);
	bool is_writable(int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int readn(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int writen(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	void close();
	void shutdown(int);

	std::string str(bool brief = true) const;
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

inline std::ostream &
operator<<(std::ostream &os, const socket &s)
{
	os << s.str(false);
	return os;
}

} // namespace net
} // namespace snf

#endif // _SNF_SOCK_H_
