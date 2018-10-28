#ifndef _SNF_ADDRINFO_H_
#define _SNF_ADDRINFO_H_

#include "net.h"

namespace snf {
namespace net {
namespace internal {

/*
 * Get address info. 10 retries with 5 seconds sleep are
 * attempted. On failure, std::system_error exception is thrown.
 */
void get_address_info(const char *, const char *, struct addrinfo *, struct addrinfo **);

} // namespace internal
} // namespace net
} // namespace snf

#endif // _SNF_ADDRINFO_H_
