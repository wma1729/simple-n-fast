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

/* Are two host name/address same? */
bool
hosteq(const std::string &h1, const std::string &h2)
{
	if (snf::streq(h1, h2))
		return true;

	int flags1 = AI_ADDRCONFIG;
	try {
		internet_address ia1 { h1 };
		flags1 |= AI_NUMERICHOST;
	} catch (std::runtime_error) {
	}

	int flags2 = AI_ADDRCONFIG;
	try {
		internet_address ia2 { h2 };
		flags2 |= AI_NUMERICHOST;
	} catch (std::runtime_error) {
	}

	host host1 { h1, flags1 };
	host host2 { h2, flags2 };

	const std::vector<std::string> &names1 = host1.get_names();
	const std::vector<std::string> &names2 = host2.get_names();

	std::vector<std::string>::const_iterator I;
	for (I = names1.begin(); I != names1.end(); ++I)
		if (std::find(names2.begin(), names2.end(), *I) != names2.end())
			return true;

	const std::vector<internet_address> &ias1 = host1.get_internet_addresses();
	const std::vector<internet_address> &ias2 = host2.get_internet_addresses();

	std::vector<internet_address>::const_iterator J;
	for (J = ias1.begin(); J != ias1.end(); ++J)
		if (std::find(ias2.begin(), ias2.end(), *J) != ias2.end())
			return true;

	return false;
}

} // namespace net
} // namespace snf
