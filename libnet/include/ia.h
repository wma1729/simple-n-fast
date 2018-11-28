#ifndef _SNF_IA_H_
#define _SNF_IA_H_

#include <string>
#include <sstream>
#include <vector>
#include "netplat.h"

namespace snf {
namespace net {

/*
 * This class encapsulates the internet address. It holds
 * - address type: AF_INET or AF_INET6
 * - binary address: in_addr or in6_addr
 */
class internet_address
{
private:
	int     m_type;
	union {
		in_addr  v4_addr;
		in6_addr v6_addr;
	} m_addr;

public:
	internet_address(const in_addr &);
	internet_address(const in6_addr &);
	internet_address(int, const std::string &);
	internet_address(const std::string &);
	internet_address(const internet_address &);

	const internet_address & operator=(const internet_address &);
	bool operator==(const internet_address &) const;

	bool is_ipv4() const { return (m_type == AF_INET); }
	bool is_ipv6() const { return (m_type == AF_INET6); }

	const in_addr *get_ipv4() const;
	const in6_addr *get_ipv6() const;

	std::string str(bool brief = true) const;
};

inline std::ostream &
operator<<(std::ostream &os, const internet_address &ia)
{
	os << ia.str(false);
	return os;
}

} // namespace net
} // namespace snf

#endif // _SNF_IA_H_
