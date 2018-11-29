#ifndef _SNF_HOST_H_
#define _SNF_HOST_H_

#include <string>
#include <vector>
#include "ia.h"

namespace snf {
namespace net {

/*
 * Performs host lookup.
 */
class host
{
private:
	std::string                     m_canonical;
	std::vector<std::string>        m_names;
	std::vector<internet_address>   m_ias;

	void add_name(const std::string &);
	void init(const std::string &, int);

public:
	host(const std::string &);
	host(const std::string &, int);

	const std::string &get_canonical_name() const { return m_canonical; }
	const std::vector<std::string> &get_names() const { return m_names; }
	const std::vector<internet_address> &get_internet_addresses() const { return m_ias; }
};

/* Determines if two hosts are equal */
bool hosteq(const std::string &, const std::string &);

} // namespace net
} // namespace snf

#endif // _SNF_HOST_H_
