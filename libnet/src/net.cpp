#include "net.h"
#include "logmgr.h"

namespace snf {
namespace net {

bool
initialize()
{
#if defined(_WIN32)
	WSADATA wsa_data;

	int status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (status != 0) {
		ERROR_STRM(nullptr, status)
			<< "failed to initialize Winsock DLL"
			<< snf::log::record::endl;
		return false;
	} else if ((LOBYTE(wsa_data.wVersion) != 2) || (HIBYTE(wsa_data.wVersion) != 2)) {
		DEBUG_STRM(nullptr)
			<< "failed to find appropriate Winsock DLL version"
			<< snf::log::record::endl;
		WSACleanup();
		return false;
	} else {
		std::atexit(snf::net::finalize);
	}
#endif
	return true;
}

void
finalize()
{
#if defined(_WIN32)
	WSACleanup();
#endif
}

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

void
socket::keepalive(bool enable)
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));

	if (enable) {
		value = 1;
		setopt(SOL_SOCKET, SO_KEEPALIVE, &value, vlen);
	} else {
		setopt(SOL_SOCKET, SO_KEEPALIVE, &value, vlen);
	}
}

void
socket::reuseaddr(bool set)
{
	int value = 0;
	int vlen = static_cast<int>(sizeof(value));

	if (set) {
		value = 1;
		setopt(SOL_SOCKET, SO_REUSEADDR, &value, vlen);
	} else {
		setopt(SOL_SOCKET, SO_REUSEADDR, &value, vlen);
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

} // namespace net
} // namespace snf
