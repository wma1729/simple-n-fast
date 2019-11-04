#include "sa.h"
#include "ia.h"
#include "net.h"
#include "addrinfo.h"
#include <algorithm>

namespace snf {
namespace net {

/*
 * Initialize the socket address.
 *
 * @param [in] ia   - the internet address.
 * @param [in] port - the internet port.
 *
 * @throws std::invalid_argument if the internet address is not
 *         initialized correctly.
 */
void
socket_address::init(const internet_address &ia, in_port_t port)
{
	if (ia.is_ipv4()) {
		memset(&(m_addr.v4_addr), 0, sizeof(sockaddr_in));
		m_addr.v4_addr.sin_family = AF_INET;
		m_addr.v4_addr.sin_port = ntoh(port);
		memcpy(&(m_addr.v4_addr.sin_addr), ia.get_ipv4(), sizeof(in_addr));
	} else if (ia.is_ipv6()) {
		memset(&(m_addr.v6_addr), 0, sizeof(sockaddr_in6));
		m_addr.v6_addr.sin6_family = AF_INET6;
		m_addr.v6_addr.sin6_port = ntoh(port);
		memcpy(&(m_addr.v6_addr.sin6_addr), ia.get_ipv6(), sizeof(in6_addr));
	} else {
		throw std::invalid_argument("invalid internet address");
	}
}

/*
 * Constructs the socket address.
 *
 * @param [in] ia   - the internet address.
 * @param [in] port - the internet port.
 *
 * @throws std::invalid_argument if the internet address is not
 *         initialized correctly.
 */
socket_address::socket_address(const internet_address &ia, in_port_t port)
{
	init(ia, port);
}

/*
 * Constructs the socket address.
 *
 * @param [in] type    - the internet address type.
 * @param [in] addrstr - the internet address string.
 * @param [in] port    - the internet port.
 *
 * @throws std::runtime_error if the internet address string could not be parsed.
 *         std::invalid_argument if the internet address is not initialized correctly.
 */
socket_address::socket_address(int type, const std::string &addrstr, in_port_t port)
{
	internet_address ia{type, addrstr};
	init(ia, port);
}

/*
 * Constructs the socket address.
 *
 * @param [in] addrstr - the internet address string.
 * @param [in] port    - the internet port.
 *
 * @throws std::runtime_error if the internet address string could not be parsed.
 *         std::invalid_argument if the internet address is not initialized correctly.
 */
socket_address::socket_address(const std::string &addrstr, in_port_t port)
{
	internet_address ia{addrstr};
	init(ia, port);
}

/*
 * Constructs the socket address from raw socket address. It is really
 * important to set the socket length correctly when using this form.
 *
 * @param [in] ss  - the raw socket address.
 * @param [in] len - the raw socket address length.
 *
 * @throws std::invalid_argument if the socket address length is incorrect.
 */
socket_address::socket_address(const sockaddr_storage &ss, socklen_t len)
{
	if (len == static_cast<socklen_t>(sizeof(sockaddr_in))) {
		const sockaddr_in *sin = reinterpret_cast<const sockaddr_in *>(&ss);
		memcpy(&(m_addr.v4_addr), sin, sizeof(sockaddr_in));
		m_addr.v4_addr.sin_family = AF_INET;
	} else if (len == static_cast<socklen_t>(sizeof(sockaddr_in6))) {
		const sockaddr_in6 *sin6 = reinterpret_cast<const sockaddr_in6 *>(&ss);
		memcpy(&(m_addr.v6_addr), sin6, sizeof(sockaddr_in6));
		m_addr.v6_addr.sin6_family = AF_INET6;
	} else {
		std::ostringstream oss;
		oss << "invalid address length (" << len << ")";
		throw std::invalid_argument(oss.str());
	}
}

socket_address::socket_address(const sockaddr_in &sin)
{
	memcpy(&(m_addr.v4_addr), &sin, sizeof(sockaddr_in));
}

socket_address::socket_address(const sockaddr_in6 &sin6)
{
	memcpy(&(m_addr.v6_addr), &sin6, sizeof(sockaddr_in6));
}

socket_address::socket_address(const socket_address &sa)
{
	m_addr = sa.m_addr;
}

const socket_address &
socket_address::operator=(const socket_address &sa)
{
	if (this != &sa) {
		this->m_addr = sa.m_addr;
	}
	return *this;
}

bool
socket_address::operator==(const socket_address &sa) const
{
	if (m_addr.v4_addr.sin_family != sa.m_addr.v4_addr.sin_family)
		return false;

	if (m_addr.v4_addr.sin_port != sa.m_addr.v4_addr.sin_port)
		return false;

	if (m_addr.v4_addr.sin_family == AF_INET) {
		if (m_addr.v4_addr.sin_addr.s_addr != sa.m_addr.v4_addr.sin_addr.s_addr)
			return false;
	} else if (m_addr.v6_addr.sin6_family == AF_INET6) {
		const uint8_t *p1 = m_addr.v6_addr.sin6_addr.s6_addr;
		const uint8_t *p2 = sa.m_addr.v6_addr.sin6_addr.s6_addr;
		if (memcmp(p1, p2, sizeof(in6_addr)) != 0)
			return false;
	} else {
		std::ostringstream oss;
		oss << "invalid address family (" << m_addr.v4_addr.sin_family << ")";
		throw std::invalid_argument(oss.str());
	}

	return true;
}

const in_addr *
socket_address::get_ipv4() const
{
	if (is_ipv4())
		return &(m_addr.v4_addr.sin_addr);
	return nullptr;
}

const in6_addr *
socket_address::get_ipv6() const
{
	if (is_ipv6())
		return &(m_addr.v6_addr.sin6_addr);
	return nullptr;
}

const sockaddr_in *
socket_address::get_sa_v4(socklen_t *len) const
{
	if (is_ipv4()) {
		if (len)
			*len = static_cast<socklen_t>(sizeof(sockaddr_in));
		return &(m_addr.v4_addr);
	}
	return nullptr;
}

const sockaddr_in6 *
socket_address::get_sa_v6(socklen_t *len) const
{
	if (is_ipv6()) {
		if (len)
			*len = static_cast<socklen_t>(sizeof(sockaddr_in6));
		return &(m_addr.v6_addr);
	}
	return nullptr;
}

const sockaddr *
socket_address::get_sa(socklen_t *len) const
{
	if (is_ipv4()) {
		if (len) *len = static_cast<socklen_t>(sizeof(sockaddr_in));
		return reinterpret_cast<const sockaddr *>(&(m_addr.v4_addr));
	} else if (is_ipv6()) {
		if (len) *len = static_cast<socklen_t>(sizeof(sockaddr_in6));
		return reinterpret_cast<const sockaddr *>(&(m_addr.v6_addr));
	}
	return nullptr;
}

in_port_t
socket_address::port() const
{
	return narrow_cast<in_port_t>(ntoh(m_addr.v4_addr.sin_port));
}

void
socket_address::port(in_port_t p)
{
	m_addr.v4_addr.sin_port = hton(p);
}

/*
 * Gets the string representation of the socket address.
 *
 * @param [in] brief - Use brief mode [Default is true.]
 *
 * @return string representation of the socket address.
 *
 * @throws std::system_error in case of failure.
 */
std::string
socket_address::str(bool brief) const
{
	char        addrstr[INET6_ADDRSTRLEN];
	const char  *paddr = nullptr;
	int         len = 0;
	std::string s;

	if (is_ipv4()) {
		len = INET_ADDRSTRLEN;
		paddr = inet_ntop(AF_INET, get_ipv4(), addrstr, len);
	} else if (is_ipv6()) {
		len = INET6_ADDRSTRLEN;
		paddr = inet_ntop(AF_INET6, get_ipv6(), addrstr, len);
	}

	if (paddr == nullptr) {
                throw std::system_error(snf::system_error(),
					std::system_category(),
					"inet_ntop() failed");
	}

	std::ostringstream oss;

	if (brief) {
		oss << paddr << "-" << port();
		s = oss.str();
	} else {

		oss << "family=";
		if (is_ipv4()) {
			oss << "inet";
		} else if (is_ipv6()) {
			oss << "inet6";
		} else {
			oss << "unknown";
		} 

		oss << ", addr=" << paddr << ", port=" << port();
		s = oss.str();
	}

	return s;
}

/*
 * Get socket address(es) ready to be bound.
 *
 * @param [in] family - the internet address family.
 * @param [in] type   - the socket type: tcp or udp.
 * @param [in] svc    - the service name. It could also be the port string.
 *
 * The flags used for getaddrinfo are AI_PASSIVE | AI_ADDRCONFIG.
 * Following bits are conditionally added to the flags:
 * - AI_V4MAPPED: if the address family is AF_INET6.
 * - AI_NUMERICSERV: if the 'svc' is a port string.
 *
 * @return a vector of socket address(es).
 *
 * @throws std::invalid_argument if svc is empty.
 *         std::system_exception if address lookup fails.
 */
std::vector<socket_address>
socket_address::get_server(int family, socket_type type, const std::string &svc)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;

	if (svc.empty())
		throw std::invalid_argument("service/port must be specified");

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	if (family == AF_INET6)
		hints.ai_flags |= AI_V4MAPPED;

	std::string::const_iterator I;
	I = std::find_if(svc.begin(), svc.end(), [](char c) { return !std::isdigit(c); });
	if (I == svc.end()) {
		hints.ai_flags |= AI_NUMERICSERV;
	}

	hints.ai_family = family;
	hints.ai_socktype = (type == socket_type::tcp) ? SOCK_STREAM : SOCK_DGRAM;

	snf::net::internal::get_address_info(nullptr, svc.c_str(), &hints, &res);

	std::vector<socket_address> sa_vec;

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(res->ai_addr);
			socket_address sa { *sin };
			sa_vec.push_back(sa);
		} else if (res->ai_family == AF_INET6) {
			sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(res->ai_addr);
			socket_address sa { *sin6 };
			sa_vec.push_back(sa);
		}
	}

	freeaddrinfo(res);

	return sa_vec;
}

std::vector<socket_address>
socket_address::get_server(int family, socket_type type, in_port_t port)
{
	std::string portstr = std::to_string(port);
	return get_server(family, type, portstr);
}

/*
 * Get socket address(es) ready to connect.
 *
 * @param [in] family - the internet address family.
 * @param [in] type   - the socket type: tcp or udp.
 * @param [in] host   - the host name to connect to.
 * @param [in] svc    - the service name. It could also be the port string.
 *
 * The flags used for getaddrinfo is AI_ADDRCONFIG.
 * Following bits are conditionally added to the flags:
 * - AI_V4MAPPED: if the address family is AF_INET6.
 * - AI_NUMERICHOST: if host is an IP address.
 * - AI_NUMERICSERV: if the 'svc' is a port string.
 *
 * @return a vector of socket address(es).
 *
 * @throws std::invalid_argument if svc is empty.
 *         std::system_exception if address lookup fails.
 */
std::vector<socket_address>
socket_address::get_client(int family, socket_type type,
			const std::string &host, const std::string &svc)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;

	if (svc.empty())
		throw std::invalid_argument("service/port must be specified");

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_ADDRCONFIG;

	if (family == AF_INET6)
		hints.ai_flags |= AI_V4MAPPED;

	try {
		internet_address ia { host };
		hints.ai_flags |= AI_NUMERICHOST;
	} catch (std::runtime_error &) {
	}

	std::string::const_iterator I;
	I = std::find_if(svc.begin(), svc.end(), [](char c) { return !std::isdigit(c); });
	if (I == svc.end()) {
		hints.ai_flags |= AI_NUMERICSERV;
	}

	hints.ai_family = family;
	hints.ai_socktype = (type == socket_type::tcp) ? SOCK_STREAM : SOCK_DGRAM;

	snf::net::internal::get_address_info(host.c_str(), svc.c_str(), &hints, &res);

	std::vector<socket_address> sa_vec;

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(res->ai_addr);
			socket_address sa { *sin };
			sa_vec.push_back(sa);
		} else if (res->ai_family == AF_INET6) {
			sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(res->ai_addr);
			socket_address sa { *sin6 };
			sa_vec.push_back(sa);
		}
	}

	freeaddrinfo(res);

	return sa_vec;
}

std::vector<socket_address>
socket_address::get_client(int family, socket_type type,
			const std::string &host, in_port_t port)
{
	std::string portstr = std::to_string(port);
	return get_client(family, type, host, portstr);
}

} // namespace net
} // namespace snf
