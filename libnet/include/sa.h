#ifndef _SNF_SA_H_
#define _SNF_SA_H_

#include <string>
#include <sstream>
#include "netplat.h"

namespace snf {
namespace net {

class internet_address;

class socket_address
{
private:
	union {
		sockaddr_in     v4_addr;
		sockaddr_in6    v6_addr;
	} m_addr;

	void init(const internet_address &, in_port_t);

public:
	socket_address(const internet_address &, in_port_t);
	socket_address(int, const std::string &, in_port_t);
	socket_address(const std::string &, in_port_t);
	socket_address(const struct sockaddr_in &);
	socket_address(const struct sockaddr_in6 &);
	socket_address(const socket_address &);

	const socket_address & operator=(const socket_address &);
	bool operator==(const socket_address &) const;

	bool is_ipv4() const { return (m_addr.v4_addr.sin_family == AF_INET); }
	bool is_ipv6() const { return (m_addr.v6_addr.sin6_family == AF_INET6); }

	const struct in_addr *get_ipv4() const;
	const struct in6_addr *get_ipv6() const;

	const sockaddr_in *get_sa_v4() const;
	const sockaddr_in6 *get_sa_v6() const;
	const sockaddr *get_sa() const;

	in_port_t get_port() const;

	std::string str(bool brief = true) const;
};

inline std::ostream &
operator<<(std::ostream &os, const socket_address &sa)
{
	os << sa.str(false);
	return os;
}

} // namespace net
} // namespace snf

#endif // _SNF_SA_H_
