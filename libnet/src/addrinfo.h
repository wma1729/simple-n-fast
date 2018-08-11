#ifndef _SNF_ADDRINFO_H_
#define _SNF_ADDRINFO_H_

#include "net.h"

namespace snf {
namespace net {
namespace internal {

void get_address_info(const char *, const char *, struct addrinfo *, struct addrinfo **);

} // namespace internal
} // namespace net
} // namespace snf

#endif // _SNF_ADDRINFO_H_
