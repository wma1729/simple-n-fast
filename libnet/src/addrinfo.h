#ifndef _SNF_ADDRINFO_H_
#define _SNF_ADDRINFO_H_

#include <string>
#include <vector>
#include "ia.h"

namespace snf {
namespace net {

std::vector<internet_address> get_internet_addresses(const std::string &);
std::string get_canonical_name(const std::string &);

} // namespace net
} // namespace snf

#endif // _SNF_ADDRINFO_H_
