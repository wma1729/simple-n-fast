#include "sock.h"

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
	// SO_RCVTIMEO not supported with getsockopt
	return -1L;
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
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));

	if (to > 1000)
		value.tv_sec = to / 1000;
	value.tv_usec = (to % 1000) * 1000;

#endif
	setopt(SOL_SOCKET, SO_RCVTIMEO, &value, vlen);
}

int64_t
socket::sndtimeout()
{
#if defined(_WIN32)
	// SO_SNDTIMEO not supported with getsockopt
	return -1L;
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
#else
	struct timeval value;
	int vlen = static_cast<int>(sizeof(value));

	if (to > 1000)
		value.tv_sec = to / 1000;
	value.tv_usec = (to % 1000) * 1000;

#endif
	setopt(SOL_SOCKET, SO_SNDTIMEO, &value, vlen);
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

void
socket::blocking(bool blk)
{
	int status;
	const char *mode = blk ? "blocking" : "non-blocking";

#if defined(_WIN32)
	u_long nb = blk ? 0 : 1;
	status = ioctlsocket(m_sock, FIONBIO, &nb);
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

	status = fcntl(m_sock, F_SETFL, flags);
#endif

	if (SOCKET_ERROR == status) {
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

} // namespace net
} // namespace snf
