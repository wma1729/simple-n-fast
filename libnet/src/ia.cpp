#include "ia.h"
#include "net.h"
#include "addrinfo.h"
#include <stdexcept>
#include <cstring>

namespace snf {
namespace net {

internet_address::internet_address(const in_addr &addr)
	: m_type(AF_INET)
{
	m_addr.v4_addr = addr;
}

internet_address::internet_address(const in6_addr &addr)
	: m_type(AF_INET6)
{
	m_addr.v6_addr = addr;
}

internet_address::internet_address(int type, const std::string &addrstr)
	: m_type(type)
{
	int r;
	std::ostringstream oss;

	if (m_type == AF_INET) {
		r = inet_pton(type, addrstr.c_str(), &(m_addr.v4_addr));
	} else if (m_type == AF_INET6) {
		r = inet_pton(type, addrstr.c_str(), &(m_addr.v6_addr));
	} else {
		r = -1;
	}

	if (r == -1) {
		oss << "invalid address family (" << m_type << ")";
	} else if (r == 0) {
		oss << "invalid address (" << addrstr << ")";
	}

	if (r != 1)
		throw std::runtime_error(oss.str());
}

internet_address::internet_address(const std::string &addrstr)
{
	in_addr      i4a;
	in6_addr     i6a;

	int r = inet_pton(AF_INET, addrstr.c_str(), &i4a);
	if (r == 1) {
		m_type = AF_INET;
		m_addr.v4_addr = i4a;
	} else {
		r = inet_pton(AF_INET6, addrstr.c_str(), &i6a);
		if (r == 1) {
			m_type = AF_INET6;
			m_addr.v6_addr = i6a;
		}
	}

	if (r != 1) {
		std::ostringstream  oss;
		oss << "invalid address (" << addrstr << ")";
		throw std::runtime_error(oss.str());
	}
}

internet_address::internet_address(const internet_address &ia)
{
	this->m_type = ia.m_type;
	if (ia.m_type == AF_INET) {
		this->m_addr.v4_addr = ia.m_addr.v4_addr;
	} else if (ia.m_type == AF_INET6) {
		this->m_addr.v6_addr = ia.m_addr.v6_addr;
	}
}

const internet_address &
internet_address::operator=(const internet_address &ia)
{
	if (this != &ia) {
		this->m_type = ia.m_type;
		if (ia.m_type == AF_INET) {
			this->m_addr.v4_addr = ia.m_addr.v4_addr;
		} else if (ia.m_type == AF_INET6) {
			this->m_addr.v6_addr = ia.m_addr.v6_addr;
		}
	}
	return *this;
}

bool
internet_address::operator==(const internet_address &ia) const
{
	if (m_type != ia.m_type)
		return false;

	if (m_type == AF_INET)
		return (m_addr.v4_addr.s_addr == ia.m_addr.v4_addr.s_addr);

	if (m_type == AF_INET6) {
		const uint8_t *p1 = m_addr.v6_addr.s6_addr;
		const uint8_t *p2 = ia.m_addr.v6_addr.s6_addr;
		return (memcmp(p1, p2, sizeof(in6_addr)) == 0);
	}

	return false;
}

const in_addr *
internet_address::get_ipv4() const
{
	if (is_ipv4())
		return &(m_addr.v4_addr);
	return nullptr;
}

const in6_addr *
internet_address::get_ipv6() const
{
	if (is_ipv6())
		return &(m_addr.v6_addr);
	return nullptr;
}

std::string
internet_address::get_canonical_name() const
{
	std::string addrstr = std::move(str(true));
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;
	std::string hostname;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snf::net::internal::get_address_info(addrstr.c_str(), nullptr, &hints, &res);

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_canonname) {
			hostname = ptr->ai_canonname;
			break;
		}
	}

	freeaddrinfo(res);
	return hostname;
}

std::string
internet_address::str(bool brief) const
{
	char        addrstr[INET6_ADDRSTRLEN];
	const char  *paddr = nullptr;
	int         len = 0;
	std::string s;

	if (m_type == AF_INET) {
		len = INET_ADDRSTRLEN;
		paddr = inet_ntop(m_type, &(m_addr.v4_addr), addrstr, len);
	} else if (m_type == AF_INET6) {
		len = INET6_ADDRSTRLEN;
		paddr = inet_ntop(m_type, &(m_addr.v6_addr), addrstr, len);
	}

	if (paddr == nullptr) {
                throw std::system_error(snf::net::error(),
					std::system_category(),
					"inet_ntop() failed");
	}

	if (brief) {
		s = paddr;
	} else {
		std::ostringstream oss;

		oss << "family=";
		if (m_type == AF_INET) {
			oss << "inet";
		} else if (m_type == AF_INET6) {
			oss << "inet6";
		} else {
			oss << "unknown";
		} 

		oss << ", addr=" << paddr;
		s = oss.str();
	}

	return s;
}

std::vector<internet_address>
internet_address::get(const std::string &host)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snf::net::internal::get_address_info(host.c_str(), nullptr, &hints, &res);

	std::vector<internet_address> ia_vec;

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
			internet_address ia { sin->sin_addr };
			ia_vec.push_back(ia);
		} else if (ptr->ai_family == AF_INET6) {
			sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(ptr->ai_addr);
			internet_address ia { sin6->sin6_addr };
			ia_vec.push_back(ia);
		}
	}

	freeaddrinfo(res);
	return ia_vec;
}

} // namespace net
} // namespace snf
