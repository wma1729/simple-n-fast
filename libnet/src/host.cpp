#include <algorithm>
#include "host.h"
#include "addrinfo.h"

namespace snf {
namespace net {

/*
 * Add name if it does not already exist.
 *
 * @param [in] n - the host name.
 */
void
host::add_name(const std::string &n)
{
	if (std::find(m_names.begin(), m_names.end(), n) == m_names.end())
		m_names.push_back(n);
}

/*
 * Initialize the host object.
 *
 * @param [in] h        - the host name.
 * @param [in] ai_flags - the address info flags.
 *
 * @throws std::system_exception in case of failure.
 */
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

/*
 * Constructs host object with the host name. The address info
 * flag used is AI_CANONNAME.
 *
 * @param [in] h - the host name.
 *
 * @throws std::system_exception in case of failure.
 */
host::host(const std::string &h)
{
	init(h, AI_CANONNAME);
}

/*
 * Constructs host object with the host name and the address info
 * flag specified.
 *
 * @param [in] h        - the host name.
 * @param [in] ai_flags - the address info flags.
 *
 * @throws std::system_exception in case of failure.
 */
host::host(const std::string &h, int ai_flags)
{
	init(h, ai_flags);
}

/*
 * Are two host name/address same?
 * It is difficult to determine if the two names/addresses
 * are for the same host. This function simply finds all
 * addresses for the two hosts and see if there is a common
 * address between them. It might be useful in certain
 * situation where the mapping between name and address is
 * static over a longer period.
 *
 * @param [in] h1 - the first host name.
 * @param [in] h2 - the second host name.
 *
 * @return true if the two hosts are same, false otherwise.
 *
 * @throws std::system_exception in case of failure.
 */
bool
hosteq(const std::string &h1, const std::string &h2)
{
	if (snf::streq(h1, h2))
		return true;

	int flags1 = AI_ADDRCONFIG;
	int flags2 = AI_ADDRCONFIG;

	try {
		internet_address ia1 { h1 };
		flags1 |= AI_NUMERICHOST;
	} catch (std::runtime_error &) {
	}

	try {
		internet_address ia2 { h2 };
		flags2 |= AI_NUMERICHOST;
	} catch (std::runtime_error &) {
	}

	host host1 { h1, flags1 };
	host host2 { h2, flags2 };

	const std::vector<std::string> &names1 = host1.get_names();
	const std::vector<std::string> &names2 = host2.get_names();

	for (auto &name : names1)
		if (std::find(names2.begin(), names2.end(), name) != names2.end())
			return true;

	const std::vector<internet_address> &ias1 = host1.get_internet_addresses();
	const std::vector<internet_address> &ias2 = host2.get_internet_addresses();

	for (auto &ia : ias1)
		if (std::find(ias2.begin(), ias2.end(), ia) != ias2.end())
			return true;

	return false;
}

} // namespace net
} // namespace snf
