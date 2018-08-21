#include <algorithm>
#include "host.h"
#include "addrinfo.h"

namespace snf {
namespace net {

void
host::add_name(const std::string &n)
{
	if (std::find(m_names.begin(), m_names.end(), n) == m_names.end())
		m_names.push_back(n);
}

void
host::init(const std::string &h, int ai_flags)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = ai_flags;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	snf::net::internal::get_address_info(h.c_str(), nullptr, &hints, &res);

	m_names.push_back(h);

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_canonname && (ptr->ai_canonname[0] != '\0')) {
			m_canonical = ptr->ai_canonname;
			add_name(ptr->ai_canonname);
		}

		if (ptr->ai_family == AF_INET) {
			sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
			internet_address ia { sin->sin_addr };
			m_ias.push_back(ia);
		} else if (ptr->ai_family == AF_INET6) {
			sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(ptr->ai_addr);
			internet_address ia { sin6->sin6_addr };
			m_ias.push_back(ia);
		}
	}

	freeaddrinfo(res);
}

host::host(const std::string &h)
{
	init(h, AI_CANONNAME);
}

host::host(const std::string &h, int ai_flags)
{
	init(h, ai_flags);
}

} // namespace net
} // namespace snf
