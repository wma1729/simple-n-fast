#include "addrinfo.h"
#include "ia.h"

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

void
get_address_info(
	const char *host,
	const char *svc, 
	struct addrinfo *hints,
	struct addrinfo **res)
{
	const int max_tries = 10;
	int cur_tries = 0;
	int status = 0;

	try {
		internet_address ia { host };
		hints->ai_flags |= AI_NUMERICHOST;
	} catch (std::runtime_error) {
	}

	do {
		status = getaddrinfo(host, svc, hints, res);
		if (status != 0) {
			if (status == EAI_AGAIN) {
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
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
