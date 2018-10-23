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

void
socket::connect_to(const socket_address &sa, int to)
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
		if (!blocking()) {
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

void
socket::bind_to(const socket_address &sa)
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

int
socket::map_system_error(int error, int default_retval)
{
	int retval = default_retval;

#if defined(_WIN32)
	if (WSAEWOULDBLOCK == error)
		retval = E_try_again;
	else if ((WSAECONNRESET == error) || (WSAECONNABORTED == error))
		retval = E_connection_reset;
	else if (WSAETIMEDOUT == error)
		retval = E_timed_out;
#else
	if ((EAGAIN == error) || (EWOULDBLOCK == error))
		retval = E_try_again;
	else if ((ECONNRESET == error) || (ECONNABORTED == error))
		retval = E_connection_reset;
	else if (ETIMEDOUT == error)
		retval = E_timed_out;
	else if (EPIPE == error)
		retval = E_broken_pipe;
#endif

	return retval;
}

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

socket &
socket::operator=(socket &&s)
{
	if (this != &s) {
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
	return *this;
}

bool
socket::keepalive()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_KEEPALIVE, &value, &vlen);
	return (value != 0);
}

void
socket::keepalive(bool enable)
{
	int value = enable ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_KEEPALIVE, &value, vlen);
}

bool
socket::reuseaddr()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_REUSEADDR, &value, &vlen);
	return (value != 0);
}

void
socket::reuseaddr(bool set)
{
	int value = set ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_REUSEADDR, &value, vlen);
}

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
		if (to == 0)
			throw std::invalid_argument("timeout value not provided for timed linger");
		value.l_onoff = 1;
		value.l_linger = to;
	}

	setopt(SOL_SOCKET, SO_LINGER, &value, vlen);
}

int
socket::rcvbuf()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_RCVBUF, &value, &vlen);
	return value;
}

void
socket::rcvbuf(int bufsize)
{
	int value = bufsize;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_RCVBUF, &value, vlen);
}

int
socket::sndbuf()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_SNDBUF, &value, &vlen);
	return value;
}

void
socket::sndbuf(int bufsize)
{
	int value = bufsize;
	int vlen = static_cast<int>(sizeof(value));
	setopt(SOL_SOCKET, SO_SNDBUF, &value, vlen);
}

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

int
socket::error()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(SOL_SOCKET, SO_ERROR, &value, &vlen);
	return value;
}

bool
socket::tcpnodelay()
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));
	getopt(IPPROTO_TCP, TCP_NODELAY, &value, &vlen);
	return (value != 0);
}

void
socket::tcpnodelay(bool nodelay)
{
	int value = nodelay ? 1 : 0;
	int vlen = static_cast<int>(sizeof(value));
	setopt(IPPROTO_TCP, TCP_NODELAY, &value, vlen);
}

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

void
socket::connect(int family, const std::string &host, in_port_t port, int to)
{
	std::vector<socket_address> sas =
		std::move(socket_address::get_client(family, m_type, host, port));

	std::vector<socket_address>::const_iterator it = sas.begin();
	if (it != sas.end())
		connect_to(*it, to);
}

void
socket::connect(const internet_address &ia, in_port_t port, int to)
{
	socket_address sa { ia, port };
	connect_to(sa, to);
}

void
socket::connect(const socket_address &sa, int to)
{
	connect_to(sa, to);
}

void
socket::bind(int family, in_port_t port)
{
	std::vector<socket_address> sas =
		std::move(socket_address::get_server(family, socket_type::tcp, port));

	std::vector<socket_address>::const_iterator it = sas.begin();
	if (it != sas.end())
		bind_to(*it);
}

void
socket::bind(const internet_address &ia, in_port_t port)
{
	socket_address sa { ia, port };
	bind_to(sa);
}

void
socket::bind(const socket_address &sa)
{
	bind_to(sa);
}

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

socket
socket::accept()
{
	sockaddr_storage saddr;
	socklen_t slen;

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

bool
socket::is_readable(int to, int *oserr)
{
	pollfd fdelem = { m_sock, POLLIN, 0 };
	std::vector<pollfd> fdvec { 1, fdelem };
	return (snf::net::poll(fdvec, to, oserr) > 0);
}

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

int
socket::write(const void *buf, int to_write, int *bwritten, int *oserr)
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
	} while (to_write > 0);

	*bwritten = nbytes;
	return retval;
}

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
