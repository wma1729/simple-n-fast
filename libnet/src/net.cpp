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
socket::getopt(int level, int optname, int *value)
{
	int retval;

#if defined(_WIN32)

	int vlen = static_cast<int>(sizeof(*value));
	retval = getsockopt(m_sock, level, optname, reinterpret_cast<char *>(value), &vlen);

#else

	socklen_t vlen = static_cast<socklen_t>(sizeof(*value));
	retval = getsockopt(m_sock, level, optname, value, &vlen);

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
socket::setopt(int level, int optname, int value)
{
	int retval;

#if defined(_WIN32)

	int vlen = static_cast<int>(sizeof(value));
	retval = setsockopt(m_sock, level, optname, reinterpret_cast<char *>(&value), vlen);

#else

	socklen_t vlen = static_cast<socklen_t>(sizeof(value));
	retval = setsockopt(m_sock, level, optname, &value, vlen);

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

	getopt(SOL_SOCKET, SO_TYPE, &value);

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
	if (enable)
		setopt(SOL_SOCKET, SO_KEEPALIVE, 1);
	else
		setopt(SOL_SOCKET, SO_KEEPALIVE, 0);
}

void
socket::reuseaddr(bool set)
{
	if (set)
		setopt(SOL_SOCKET, SO_REUSEADDR, 1);
	else
		setopt(SOL_SOCKET, SO_REUSEADDR, 0);
}

} // namespace net
} // namespace snf
