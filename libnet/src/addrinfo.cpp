#include "addrinfo.h"

#if !defined(_WIN32)
#include "aiec.h"
#endif

#include <thread>
#include <chrono>
#include <system_error>
#include <stdexcept>

namespace snf {
namespace net {
namespace internal {

constexpr int MAX_TRIES = 10;
constexpr int SLEEP_FOR = 5000;

/*
 * Gets address information. The call is retries 10 times with
 * a delay of 5 seconds if the call fails with EAI_AGAIN error.
 *
 * @param [in]  host  - the host name.
 * @param [in]  svc   - the service name.
 * @param [in]  hints - hints to the address lookup algorithm.
 * @param [out] res   - result of the lookup.
 *
 * @throw std::system_exception in case of failure.
 */
void
get_address_info(
	const char *host,
	const char *svc, 
	const struct addrinfo *hints,
	struct addrinfo **res)
{
	const int max_tries = MAX_TRIES;
	int cur_tries = 0;
	int status = 0;

	do {
		status = getaddrinfo(host, svc, hints, res);
		if (status != 0) {
			if (status == EAI_AGAIN) {
				std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_FOR));
				cur_tries++;
			}
		}
	} while ((status == EAI_AGAIN) && (cur_tries < max_tries));


	if (status != 0) {
		std::ostringstream oss;
		oss << "failed to get address for " << host;

#if defined(_WIN32)
		throw std::system_error(
			snf::net::error(),
			std::system_category(),
			oss.str());
#else
		if (status == EAI_SYSTEM) {
			throw std::system_error(
				snf::net::error(),
				std::system_category(),
				oss.str());
		} else {
			throw std::system_error(
				status,
				snf::net::gai_category(),
				oss.str());
		}
#endif
	}
}

} // namespace internal
} // namespace net
} // namespace snf
