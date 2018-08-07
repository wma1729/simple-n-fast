#include "sa.h"
#include "ia.h"
#include "net.h"

namespace snf {
namespace net {

void
socket_address::init(const internet_address &ia, in_port_t port)
{
	if (ia.is_ipv4()) {
		memset(&(m_addr.v4_addr), 0, sizeof(struct sockaddr_in));

		m_addr.v4_addr.sin_family = AF_INET;
		m_addr.v4_addr.sin_port = ntoh(port);
		memcpy(&(m_addr.v4_addr.sin_addr), ia.get_ipv4(),
			sizeof(struct in_addr));
	} else if (ia.is_ipv6()) {
		memset(&(m_addr.v6_addr), 0, sizeof(struct sockaddr_in6));

		m_addr.v6_addr.sin6_family = AF_INET6;
		m_addr.v6_addr.sin6_port = ntoh(port);
		memcpy(&(m_addr.v6_addr.sin6_addr), ia.get_ipv6(),
			sizeof(struct in6_addr));
	} else {
		throw std::runtime_error("invalid internet address");
	}
}

socket_address::socket_address(const internet_address &ia, in_port_t port)
{
	init(ia, port);
}

socket_address::socket_address(int type, const std::string &addrstr, in_port_t port)
{
	internet_address ia{type, addrstr};
	init(ia, port);
}

socket_address::socket_address(const std::string &addrstr, in_port_t port)
{
	internet_address ia{addrstr};
	init(ia, port);
}

socket_address::socket_address(const struct sockaddr_in &sin)
{
	memcpy(&(m_addr.v4_addr), &sin, sizeof(struct sockaddr_in));
}

socket_address::socket_address(const struct sockaddr_in6 &sin6)
{
	memcpy(&(m_addr.v6_addr), &sin6, sizeof(struct sockaddr_in6));
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
		if (memcmp(p1, p2, sizeof(struct in6_addr)) != 0)
			return false;
	} else {
		throw std::runtime_error("invalid address family");
	}

	return true;
}

const struct in_addr *
socket_address::get_ipv4() const
{
	if (is_ipv4())
		return &(m_addr.v4_addr.sin_addr);
	return nullptr;
}

const struct in6_addr *
socket_address::get_ipv6() const
{
	if (is_ipv6())
		return &(m_addr.v6_addr.sin6_addr);
	return nullptr;
}

const sockaddr_in *
socket_address::get_sa_v4() const
{
	if (is_ipv4())
		return &(m_addr.v4_addr);
	return nullptr;
}

const sockaddr_in6 *
socket_address::get_sa_v6() const
{
	if (is_ipv6())
		return &(m_addr.v6_addr);
	return nullptr;
}

const sockaddr *
socket_address::get_sa() const
{
	if (is_ipv4())
		return reinterpret_cast<const sockaddr *>(&(m_addr.v4_addr));
	else if (is_ipv6())
		return reinterpret_cast<const sockaddr *>(&(m_addr.v6_addr));
	return nullptr;
}

in_port_t
socket_address::get_port() const
{
	return narrow_cast<in_port_t>(ntoh(m_addr.v4_addr.sin_port));
}

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
		oss << paddr << "-" << get_port();
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

		oss << ", addr=" << paddr << ", port=" << get_port();
		s = oss.str();
	}

	return s;
}

} // namespace net
} // namespace snf
