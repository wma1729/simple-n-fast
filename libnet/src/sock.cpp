#include "sock.h"
#include "ia.h"
#include "error.h"

namespace snf {
namespace net {

const char *
socket::optstr(int level, int optname)
{
	switch (level) {
		case SOL_SOCKET:
			switch (optname) {
				case SO_TYPE: return "SO_TYPE";
				case SO_KEEPALIVE: return "SO_KEEPALIVE";
				case SO_REUSEADDR: return "SO_REUSEADDR";
				case SO_LINGER: return "SO_LINGER";
				case SO_RCVBUF: return "SO_RCVBUF";
				case SO_SNDBUF: return "SO_SNDBUF";
				case SO_RCVTIMEO: return "SO_RCVTIMEO";
				case SO_SNDTIMEO: return "SO_SNDTIMEO";
				case SO_ERROR: return "SO_ERROR";
				default: break;
			}
			break;

		case IPPROTO_TCP:
			switch (optname) {
				case TCP_NODELAY: return "TCP_NODELAY";
				default: break;
			}
			break;

		default:
			break;
	}

	return "unhandled-socket-option";
}

void
socket::getopt(int level, int optname, void *value, int *vlen)
{
	int retval;

#if defined(_WIN32)
	retval = getsockopt(m_sock, level, optname, reinterpret_cast<char *>(value), vlen);
#else
	retval = getsockopt(m_sock, level, optname, value, reinterpret_cast<socklen_t *>(vlen));
#endif

	if (SOCKET_ERROR == retval) {
		std::ostringstream oss;
		oss << "failed to get socket option " << optstr(level, optname);
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}
}

void
socket::setopt(int level, int optname, void *value, int vlen)
{
	int retval;

#if defined(_WIN32)
	retval = setsockopt(m_sock, level, optname, reinterpret_cast<char *>(value), vlen);
#else
	retval = setsockopt(m_sock, level, optname, value, static_cast<socklen_t>(vlen));
#endif

	if (SOCKET_ERROR == retval) {
		std::ostringstream oss;
		oss << "failed to set socket option " << optstr(level, optname);
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}
}

/*
 * Constructs a socket pair. This is straight-forward on Unix
 * platforms as this simply calls socketpair with family of
 * AF_UNIX and type set to SOCK_STREAM. Windows, on the other
 * hand, does not support socketpair. The implementation creates
 * a listener socket. Later two connected sockets are created
 * using connect and accept calls.
 *
 * @returns an array of connected sockets of size 2.
 *
 * @throws std::system_error in case of error.
 */
std::array<socket, 2>
socket::socketpair()
{
#ifndef _WIN32
	sock_t s[2];
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s) < 0) {
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			"failed to create socket pair");
	}

	socket s1(s[0]);
	socket s2(s[1]);
#else
	socket listener(AF_INET, socket_type::tcp);

	listener.reuseaddr(true);
	listener.bind(AF_INET, 0);
	listener.listen(1);

	const socket_address &sa = listener.local_address();

	socket s1(AF_INET, socket_type::tcp);
	s1.connect(AF_INET, "localhost", sa.port());

	socket s2 = std::move(listener.accept());
#endif

	return std::array<socket, 2> { std::move(s1), std::move(s2) };
}

/*
 * Constructs the socket object from raw socket.
 *
 * @param [in] s   - raw socket.
 *
 * @throws std::invalid_argument if
 *         - the socket type is neither SOCK_STREAM nor SOCK_DGRAM.
 *         std::system_error if the socket type could not be determined.
 */
socket::socket(sock_t s)
	: m_sock(s)
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));

	getopt(SOL_SOCKET, SO_TYPE, &value, &vlen);

	if (value == SOCK_STREAM) {
		m_type = socket_type::tcp;
	} else if (value == SOCK_DGRAM) {
		m_type = socket_type::udp;
	} else {
		throw std::invalid_argument("invalid socket type");
	}
}

/*
 * Constructs the socket object from raw socket and peer address.
 *
 * @param [in] s   - raw socket.
 * @param [in] ss  - peer socket address.
 * @param [in] len - peer socket address length.
 *
 * @throws std::invalid_argument if
 *         - the socket type is neither SOCK_STREAM nor SOCK_DGRAM.
 *         - the socket peer address length is incorrect.
 *         std::system_error if the socket type could not be determined.
 */
socket::socket(sock_t s, const sockaddr_storage &ss, socklen_t len)
	: m_sock(s)
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));

	getopt(SOL_SOCKET, SO_TYPE, &value, &vlen);

	if (value == SOCK_STREAM) {
		m_type = socket_type::tcp;
	} else if (value == SOCK_DGRAM) {
		m_type = socket_type::udp;
	} else {
		throw std::invalid_argument("invalid socket type");
	}

	m_peer = DBG_NEW socket_address(ss, len);
}

/*
 * Constructs the socket object.
 *
 * @param [in] family - internet address family.
 * @param [in] type   - socket type: tcp or udp.
 *
 * @throws std::invalid_argument if the address family is neither AF_INET nor AF_INET6.
 *         std::system_error if the socket could not be created.
 */
socket::socket(int family, socket_type type)
{
	int sock_type = SOCK_STREAM;
	int protocol = IPPROTO_TCP;
	const char *type_str = "tcp";

	if (type == socket_type::udp) {
		sock_type = SOCK_DGRAM;
		protocol = IPPROTO_UDP; 
		type_str = "udp";
	}

	const char *family_str = nullptr;
	if (family == AF_INET) {
		family_str = "inet";
	} else if (family == AF_INET6) {
		family_str = "inet6";
	} else {
		throw std::invalid_argument("invalid address family");
	}


	m_sock = ::socket(family, sock_type, protocol);
	if (m_sock == INVALID_SOCKET) {
		std::ostringstream oss;
		oss << "failed to create " << type_str
			<< " socket for " << family_str << " family";
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}

	m_type = type;
}

/* Move constructor */
socket::socket(socket &&s)
{
	m_sock = s.m_sock;
	s.m_sock = INVALID_SOCKET;

	m_type = s.m_type;

	if (s.m_local) {
		m_local = s.m_local;
		s.m_local = nullptr;
	}

	if (s.m_peer) {
		m_peer = s.m_peer;
		s.m_peer = nullptr;
	}

#if defined(_WIN32)
	m_blocking = s.m_blocking;
	m_rcvtimeo = s.m_rcvtimeo;
	m_sndtimeo = s.m_sndtimeo;
#endif
}

/*
 * Close the socket and release the memory.
 */
socket::~socket()
{
	close();
	if (m_local) {
		delete m_local;
		m_local = nullptr;
	}
	if (m_peer) {
		delete m_peer;
		m_peer = nullptr;
	}
}

/* Move operator */
socket &
socket::operator=(socket &&s)
{
	if (this != &s) {
		m_sock = s.m_sock;
		s.m_sock = INVALID_SOCKET;

		m_type = s.m_type;

		if (s.m_local) {
			if (m_local) delete m_local;
			m_local = s.m_local;
			s.m_local = nullptr;
		}

		if (s.m_peer) {
			if (m_peer) delete m_peer;
			m_peer = s.m_peer;
			s.m_peer = nullptr;
		}

#if defined(_WIN32)
		m_blocking = s.m_blocking;
		m_rcvtimeo = s.m_rcvtimeo;
		m_sndtimeo = s.m_sndtimeo;
#endif
	}
	return *this;
}

/*
 * Determines if the socket option keepalive (SO_KEEPALIVE) is enabled.
 *
 * @return true if socket keepalive is enabled, false otherwise.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
bool
socket::keepalive()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_KEEPALIVE, &value, &vlen);
	return (value != 0);
}

/*
 * Enables/disables the socket option keepalive (SO_KEEPALIVE).
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::keepalive(bool enable)
{
	int value = enable ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_KEEPALIVE, &value, vlen);
}

/*
 * Determines if the socket option reuse address (SO_REUSEADDR) is enabled.
 *
 * @return true if socket reuse address is enabled, false otherwise.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
bool
socket::reuseaddr()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_REUSEADDR, &value, &vlen);
	return (value != 0);
}

/*
 * Enables/disables the socket option reuse address (SO_REUSEADDR).
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::reuseaddr(bool set)
{
	int value = set ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_REUSEADDR, &value, vlen);
}

/*
 * Gets the linger type. There are 3 possible return values:
 * - socket::linger_type::dflt  - When the socket is closed, close() returns immediately.
 *                                The system will try to deliver pending data in send buffer
 *                                to the peer.
 * - socket::linger_type::none  - When the socket is closed, close() returns immediately.
 *                                Any pending data in send buffer is discarded and RST is sent
 *                                to the peer.
 * - socket::linger_type::timed - When the socket is closed, close() blocks until
 *                                i)  all the data in the send buffer is sent and ACK'd by the peer
 *                                ii) or the timeout expires. In this case, close() sets error
 *                                    code to EWOULDBLOCK.
 *
 * @param [out] to - timeout value if linger type is socket::linger_type::timed.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
socket::linger_type
socket::linger(int *to)
{
	struct linger value;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_LINGER, &value, &vlen);

	if (value.l_onoff == 0) {
		return socket::linger_type::dflt;
	} else {
		if (value.l_linger == 0) {
			return socket::linger_type::none;
		} else {
			if (to) *to = value.l_linger;
			return socket::linger_type::timed;
		}
	}
}

/*
 * Sets the linger type. There are 3 possible linger values:
 * - socket::linger_type::dflt  - When the socket is closed, close() returns immediately.
 *                                The system will try to deliver pending data in send buffer
 *                                to the peer.
 * - socket::linger_type::none  - When the socket is closed, close() returns immediately.
 *                                Any pending data in send buffer is discarded and RST is sent
 *                                to the peer.
 * - socket::linger_type::timed - When the socket is closed, close() blocks until
 *                                i)  all the data in the send buffer is sent and ACK'd by the peer
 *                                ii) or the timeout expires. In this case, close() sets error
 *                                    code to EWOULDBLOCK.
 *
 * @param [in] lt - linger type.
 * @param [in] to - timeout value if linger type is socket::linger_type::timed.
 *
 * @throws std::system_error if the socket option could not be set.
 *         std::invalid_argument if the linger type is socket::linger_type::timed but
 *         the timeout value is <= 0.
 */
void
socket::linger(socket::linger_type lt, int to)
{
	struct linger value;
	int vlen = static_cast<int>(sizeof(value));

	if (lt == socket::linger_type::dflt) {
		value.l_onoff = 0;
		value.l_linger = 0;
	} else if (lt == socket::linger_type::none) {
		value.l_onoff = 1;
		value.l_linger = 0;
	} else if (lt == socket::linger_type::timed) {
		if (to <= 0)
			throw std::invalid_argument("timeout value not provided for timed linger");
		value.l_onoff = 1;
		value.l_linger = to;
	}

	setopt(SOL_SOCKET, SO_LINGER, &value, vlen);
}

/*
 * Gets the socket receive buffer size (SO_RCVBUF).
 *
 * @return the socket receive buffer size.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
int
socket::rcvbuf()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_RCVBUF, &value, &vlen);
	return value;
}

/*
 * Sets the socket receive buffer size (SO_RCVBUF).
 *
 * @param [in] bufsize - socket receive buffer size.
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::rcvbuf(int bufsize)
{
	int value = bufsize;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_RCVBUF, &value, vlen);
}

/*
 * Gets the socket send buffer size (SO_SNDBUF).
 *
 * @return the socket send buffer size.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
int
socket::sndbuf()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_SNDBUF, &value, &vlen);
	return value;
}

/*
 * Sets the socket send buffer size (SO_SNDBUF).
 *
 * @param [in] bufsize - socket send buffer size.
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::sndbuf(int bufsize)
{
	int value = bufsize;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_SNDBUF, &value, vlen);
}

/*
 * Gets the socket receive timeout (SO_RCVTIMEO). Windows does not
 * support this option. So this function relies on a local variable
 * that holds the last set value.
 *
 * @return the socket receive timeout in milliseconds.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
int64_t
socket::rcvtimeout()
{
#if defined(_WIN32)
	// SO_RCVTIMEO not supported with getsockopt; rely on variable :-(
	return m_rcvtimeo;
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_RCVTIMEO, &value, &vlen);
	return value.tv_sec * 1000 + value.tv_usec / 1000;
#endif
}

/*
 * Sets the socket receive timeout (SO_RCVTIMEO). On Windows,
 * the changed value is saved in a local varible that can be
 * used by the read version of rcvtimeout().
 *
 * @param [in] to - the socket receive timeout in milliseconds.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
void
socket::rcvtimeout(int64_t to)
{
#if defined(_WIN32)
	DWORD value = narrow_cast<DWORD>(to);
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_RCVTIMEO, &value, vlen);
	m_rcvtimeo = value;
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));

	if (to > 1000)
		value.tv_sec = to / 1000;
	value.tv_usec = (to % 1000) * 1000;
	setopt(SOL_SOCKET, SO_RCVTIMEO, &value, vlen);
#endif
}

/*
 * Gets the socket send timeout (SO_SNDTIMEO). Windows does not
 * support this option. So this function relies on a local variable
 * that holds the last set value.
 *
 * @return the socket send timeout in milliseconds.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
int64_t
socket::sndtimeout()
{
#if defined(_WIN32)
	// SO_SNDTIMEO not supported with getsockopt
	return m_sndtimeo;
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_SNDTIMEO, &value, &vlen);
	return value.tv_sec * 1000 + value.tv_usec / 1000;
#endif
}

/*
 * Sets the socket send timeout (SO_SNDTIMEO). On Windows,
 * the changed value is saved in a local varible that can be
 * used by the read version of sndtimeout().
 *
 * @param [in] to - the socket send timeout in milliseconds.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
void
socket::sndtimeout(int64_t to)
{
#if defined(_WIN32)
	DWORD value = narrow_cast<DWORD>(to);
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_SNDTIMEO, &value, vlen);
	m_sndtimeo = value;
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));

	if (to > 1000)
		value.tv_sec = to / 1000;
	value.tv_usec = (to % 1000) * 1000;
	setopt(SOL_SOCKET, SO_SNDTIMEO, &value, vlen);
#endif
}

/*
 * Gets the socket error (SO_ERROR). The call also clears
 * the last socket error.
 *
 * @return the last socket error.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
int
socket::error()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_ERROR, &value, &vlen);
	return value;
}

/*
 * Determines if the tcp option no delay (TCP_NODELAY) is enabled.
 *
 * @return true if tcp option no delay is enabled, false otherwise.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
bool
socket::tcpnodelay()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(IPPROTO_TCP, TCP_NODELAY, &value, &vlen);
	return (value != 0);
}

/*
 * Enables/disables the tcp option no delay (TCP_NODELAY).
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::tcpnodelay(bool nodelay)
{
	int value = nodelay ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(IPPROTO_TCP, TCP_NODELAY, &value, vlen);
}

/*
 * Determines if the socket is in blocking mode. Windows does
 * not support this option. So this function relies on a local variable
 * that holds the last mode.
 *
 * @return true if socket is in blocking mode.
 *
 * @throws std::system_error if the socket option could not be fetched.
 */
bool
socket::blocking()
{
#if defined(_WIN32)
	return m_blocking;
#else
	int flags = fcntl(m_sock, F_GETFL, 0);
	if (SOCKET_ERROR == flags) {
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			"failed to get socket flags");
	}

	if ((flags & O_NONBLOCK) == O_NONBLOCK)
		return false;
	return true;
#endif
}

/*
 * Enables/disables the socket blocking mode. On Windows,
 * the changed value is saved in a local varible that can be
 * used by the read version of blocking().
 *
 * @param [in] blk - true makes the socket blocking,
 *                   false makes the socket non-blocking.
 *
 * @throws std::system_error if the socket option could not be set.
 */
void
socket::blocking(bool blk)
{
	int retval;
	const char *mode = blk ? "blocking" : "non-blocking";

#if defined(_WIN32)
	u_long nb = blk ? 0 : 1;
	retval = ioctlsocket(m_sock, FIONBIO, &nb);
	m_blocking = !nb;
#else
	int flags = fcntl(m_sock, F_GETFL, 0);
	if (SOCKET_ERROR == flags) {
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			"failed to get socket flags");
	}

	if (blk) {
		if ((flags & O_NONBLOCK) == O_NONBLOCK) {
			flags &= ~O_NONBLOCK;
		} else {
			// already blocking; nothing to do
			return;
		}
	} else {
		if ((flags & O_NONBLOCK) == O_NONBLOCK) {
			// already non-blocking; nothing to do
			return;
		} else {
			flags |= O_NONBLOCK;
		}
	}

	retval = fcntl(m_sock, F_SETFL, flags);
#endif

	if (SOCKET_ERROR == retval) {
		std::ostringstream oss;
		oss << "failed to set socket mode to " << mode;
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}
}

/*
 * Gets the local socket address of a connected socket.
 *
 * @return the local socket address.
 *
 * @throws std::system_error if the local address could not be obtained.
 */
const socket_address &
socket::local_address()
{
	if (m_local == nullptr) {
		struct sockaddr_storage ss;
		socklen_t len = static_cast<socklen_t>(sizeof(ss));

		if (getsockname(m_sock, reinterpret_cast<sockaddr *>(&ss), &len) == SOCKET_ERROR) {
			std::ostringstream oss;
			oss << "failed to get local socket address for "
				<< static_cast<int64_t>(m_sock);
			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		}

		m_local = DBG_NEW socket_address(ss, len);
	}

	return *m_local;
}

/*
 * Gets the remote socket address of a connected socket.
 *
 * @return the remote socket address.
 *
 * @throws std::system_error if the remote address could not be obtained.
 */
const socket_address &
socket::peer_address()
{
	if (m_peer == nullptr) {
		struct sockaddr_storage ss;
		socklen_t len = static_cast<socklen_t>(sizeof(ss));

		if (getpeername(m_sock, reinterpret_cast<sockaddr *>(&ss), &len) == SOCKET_ERROR) {
			std::ostringstream oss;
			oss << "failed to get local socket address for "
				<< static_cast<int64_t>(m_sock);
			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		}

		m_peer = DBG_NEW socket_address(ss, len);
	}

	return *m_peer;
}

/*
 * Connects to the <host>:<port> using the specified address family. The socket
 * address(es) are obtained using socket_address::get_client() and this call
 * attempts to connect to the first socket address.
 *
 * @param [in] family - internet address family.
 * @param [in] host   - remote host.
 * @param [in] port   - remote port.
 * @param [in] to     - connect timeout in milliseconds.
 *                      POLL_WAIT_FOREVER for inifinite wait.
 *                      POLL_WAIT_NONE for no wait.
 *
 * @throws std::system_error if the connection could not be established.
 */
void
socket::connect(int family, const std::string &host, in_port_t port, int to)
{
	std::vector<socket_address> sas =
		std::move(socket_address::get_client(family, m_type, host, port));

	if (!sas.empty())
		connect(sas[0], to);
}

/*
 * Connects to the <intenet_address>:<port>.
 *
 * @param [in] ia - internet address.
 * @param [in] to - connect timeout in milliseconds.
 *                  POLL_WAIT_FOREVER for inifinite wait.
 *                  POLL_WAIT_NONE for no wait.
 *
 * @throws std::system_error if the connection could not be established.
 */
void
socket::connect(const internet_address &ia, in_port_t port, int to)
{
	socket_address sa { ia, port };
	connect(sa, to);
}

/*
 * Connects to the remote host.
 *
 * @param [in] sa - socket address of the the remote host.
 * @param [in] to - connect timeout in milliseconds.
 *                  POLL_WAIT_FOREVER for inifinite wait.
 *                  POLL_WAIT_NONE for no wait.
 *
 * @throws std::system_error if the connection could not be established.
 */
void
socket::connect(const socket_address &sa, int to)
{
	int retval = 0;
	socklen_t len = 0;
	const sockaddr *saddr = sa.get_sa(&len);

	if ((POLL_WAIT_FOREVER == to) && blocking()) {
		retval = ::connect(m_sock, saddr, len);
		if (SOCKET_ERROR == retval) {
			std::ostringstream oss;
			oss << "failed to connect to " << sa;
			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		}
	} else {
		bool reset = false;
		if (blocking()) {
			reset = true;
			blocking(false);
		}

		retval = ::connect(m_sock, saddr, len);
		if (SOCKET_ERROR == retval) {
			retval = snf::net::error();
			if (connect_in_progress(retval)) {
				pollfd fdelem = { m_sock, POLLOUT | POLLERR, 0 };
				std::vector<pollfd> fdvec { 1, fdelem };

				int syserr;
				retval = snf::net::poll(fdvec, to, &syserr);
				if (SOCKET_ERROR == retval) {
					retval = syserr;
				} else if (retval == 0) {
					retval = ETIMEDOUT;
				} else {
					if (fdvec[0].revents & POLLERR)
						retval = error();
					else
						retval = 0;
				}

				if (retval != 0) {
					if (reset)
						blocking(true);

					std::ostringstream oss;
					oss << "failed to connect to " << sa;
					throw std::system_error(
						retval,
						std::system_category(),
						oss.str());
				}
			}
		}

		if (reset)
			blocking(true);
	}
}

/*
 * Binds to <localhost>:<port> using the specified address family. The socket
 * address(es) are obtained using socket_address::get_server() and this call
 * attempts to bind to the first socket address.
 *
 * @param [in] family - internet address family.
 * @param [in] port   - local internet port.
 *
 * @throws std::system_error if the socket could not be bound.
 */
void
socket::bind(int family, in_port_t port)
{
	std::vector<socket_address> sas =
		std::move(socket_address::get_server(family, socket_type::tcp, port));

	if (!sas.empty())
		bind(sas[0]);
}

/*
 * Binds to the <intenet_address>:<port>.
 *
 * @param [in] ia - internet address.
 * @param [in] port - local internet port.
 *
 * @throws std::system_error if the socket could not be bound.
 */
void
socket::bind(const internet_address &ia, in_port_t port)
{
	socket_address sa { ia, port };
	bind(sa);
}

/*
 * Binds socket to the local socket address.
 *
 * @param [in] sa - socket address of the the remote host.
 *
 * @throws std::system_error if the socket could not be bound.
 */
void
socket::bind(const socket_address &sa)
{
	socklen_t len = 0;
	const sockaddr *saddr = sa.get_sa(&len);

	int retval = ::bind(m_sock, saddr, len);
	if (SOCKET_ERROR == retval) {
		std::ostringstream oss;
		oss << "failed to bind to " << sa;
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}
}

/*
 * Start listening on the socket.
 *
 * @param [in] backlog - the backlog size. Value lower than 5 are
 *                       bumped up to 5.
 *
 * @throws std::system_error if the underlying ::listen() call fails.
 */
void
socket::listen(int backlog)
{
	if (backlog < 5)
		backlog = 5;
	if (backlog > SOMAXCONN)
		backlog = SOMAXCONN;

	if (SOCKET_ERROR == ::listen(m_sock, backlog)) {
		std::ostringstream oss;
		oss << "failed to listen on socket " << static_cast<int64_t>(m_sock)
			<< " with a backlog of " << backlog;
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}
}

/*
 * Accepts a request and returns a new accepted socket. The new
 * socket has the peer address set.
 *
 * @returns new accepted socket.
 *
 * @throws std::system_error if the underlying ::accept() call fails.
 */
socket
socket::accept()
{
	sockaddr_storage saddr = { 0 };
	socklen_t slen = static_cast<socklen_t>(sizeof(saddr));

	sock_t s = ::accept(
			m_sock,
			reinterpret_cast<sockaddr *>(&saddr),
			&slen);
	if (INVALID_SOCKET == s) {
		std::ostringstream oss;
		oss << "failed to accept socket " << static_cast<int64_t>(m_sock);
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
	}

	return socket {s, saddr, slen} ;
}

/*
 * Determines if the socket is readable in the specified time.
 *
 * @param [in]  to    - timeout in milliseconds.
 *                      POLL_WAIT_FOREVER for inifinite wait.
 *                      POLL_WAIT_NONE for no wait.
 * @param [out] oserr - system error in case of failure.
 *
 * @return true if the socket is readable, false otherwise.
 */
bool
socket::is_readable(int to, int *oserr)
{
	pollfd fdelem = { m_sock, POLLIN, 0 };
	std::vector<pollfd> fdvec { 1, fdelem };
	return (snf::net::poll(fdvec, to, oserr) > 0);
}

/*
 * Determines if the socket is writable in the specified time.
 *
 * @param [in]  to    - timeout in milliseconds.
 *                      POLL_WAIT_FOREVER for inifinite wait.
 *                      POLL_WAIT_NONE for no wait.
 * @param [out] oserr - system error in case of failure.
 *
 * @return true if the socket is writable, false otherwise.
 */
bool
socket::is_writable(int to, int *oserr)
{
	pollfd fdelem = { m_sock, POLLOUT, 0 };
	std::vector<pollfd> fdvec { 1, fdelem };
	return (snf::net::poll(fdvec, to, oserr) > 0);
}

/**
 * Reads from the socket.
 *
 * @param [out] buf     - buffer to read the data into.
 * @param [in]  to_read - number of bytes to read.
 * @param [out] bread   - number of bytes read. This
 *                        can be less than to_read.
 * @param [in]  to      - timeout in milliseconds.
 *                        POLL_WAIT_FOREVER for inifinite wait.
 *                        POLL_WAIT_NONE for no wait.
 * @param [out] oserr   - system error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
socket::read(void *buf, int to_read, int *bread, int to, int *oserr)
{
	int     retval = E_ok;
	int     n = 0, nbytes = 0;
	char    *cbuf = static_cast<char *>(buf);

	if (buf == nullptr)
		return E_invalid_arg;

	if (to_read <= 0)
		return E_invalid_arg;

	if (bread == nullptr)
		return E_invalid_arg;

	do {
		if (is_readable(to, oserr)) {
			n = ::recv(m_sock, cbuf, to_read, 0);
			if (SOCKET_ERROR == n) {
				int error = snf::net::error();
#if !defined(_WIN32)
				if (EINTR == error)
					continue;
#endif
				if (oserr) *oserr = error;
				retval = map_system_error(error, E_read_failed);
				break;
			} else if (0 == n) {
				break;
			} else {
				cbuf += n;
				to_read -= n;
				nbytes += n;
			}
		} else {
			retval = map_system_error(*oserr, E_read_failed);
		}
	} while (to_read > 0);

	*bread = nbytes;
	return retval;
}

/**
 * Writes to the socket. There is no need to handle SIGPIPE
 * explicitly while using this.
 *
 * @param [in]  buf      - buffer to write the data from.
 * @param [in]  to_write - number of bytes to write.
 * @param [out] bwritten - number of bytes written.
 * @param [in]  to       - timeout in milliseconds.
 *                         POLL_WAIT_FOREVER for inifinite wait.
 *                         POLL_WAIT_NONE for no wait.
 * @param [out] oserr    - system error code.
 *
 * @return E_ok on success, -ve error code on success.
 */
int
socket::write(const void *buf, int to_write, int *bwritten, int to, int *oserr)
{
	int         retval = E_ok;
	int         flags = 0;
	int         n = 0, nbytes = 0;
	const char  *cbuf = static_cast<const char *>(buf);

	if (buf == nullptr)
		return E_invalid_arg;

	if (to_write <= 0)
		return E_invalid_arg;

	if (bwritten == nullptr)
		return E_invalid_arg;

#if !defined(_WIN32)
	flags = MSG_NOSIGNAL;
#endif

	do {
		if (is_writable(to, oserr)) {
			n = ::send(m_sock, cbuf, to_write, flags);
			if (SOCKET_ERROR == n) {
				int error = snf::net::error();
#if !defined(_WIN32)
				if (EINTR == error)
					continue;
#endif
				if (oserr) *oserr = error;
				retval = map_system_error(error, E_write_failed);
				break;
			} else if (0 == n) {
				break;
			} else {
				cbuf += n;
				to_write -= n;
				nbytes += n;
			}
		} else {
			retval = map_system_error(*oserr, E_write_failed);
		}
	} while (to_write > 0);

	*bwritten = nbytes;
	return retval;
}

/*
 * Closes the socket.
 *
 * @throws std::system_error if the socket could not be closed.
 */
void
socket::close()
{
	if (m_sock != INVALID_SOCKET) {
#if defined(_WIN32)
		int retval = ::closesocket(m_sock);
#else
		int retval = ::close(m_sock);
#endif
		if (SOCKET_ERROR == retval) {
			std::ostringstream oss;
			oss << "failed to close socket " << static_cast<int64_t>(m_sock);
			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		} else {
			m_sock = INVALID_SOCKET;
		}
	}
}

/*
 * Shuts down the socket.
 *
 * @param [in] type - SHUTDOWN_READ (close the read side)
 *                    SHUTDOWN_WRITE (close the write side)
 *                    SHUTDOWN_RDWR (close both side)
 *
 * @throws std::system_error if the socket could not be shutdown.
 */
void
socket::shutdown(int type)
{
	if (m_sock != INVALID_SOCKET) {
		if (::shutdown(m_sock, type) == SOCKET_ERROR) {
			const char *typestr;

			if (type == SHUTDOWN_READ)
				typestr = "read";
			else if (type == SHUTDOWN_WRITE)
				typestr = "write";
			else if (type == SHUTDOWN_RDWR)
				typestr = "read-write";
			else
				typestr = "unknown";

			std::ostringstream oss;
			oss << "failed to shutdown(" << typestr << ") socket "
				<< static_cast<int64_t>(m_sock);

			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		}
	}
}

} // namespace net
} // namespace snf
