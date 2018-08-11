#include "addrinfo.h"
#include "net.h"
#include <thread>
#include <chrono>

#if !defined(_WIN32)
#include "aiec.h"
#endif

namespace snf {
namespace net {

static void
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

std::vector<internet_address>
get_internet_addresses(const std::string &host)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	get_address_info(host.c_str(), nullptr, &hints, &res);

	std::vector<internet_address> ia_vec;

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) {
			sockaddr_in *sin = reinterpret_cast<sockaddr_in *>(ptr->ai_addr);
			internet_address ia { sin->sin_addr };
			ia_vec.push_back(ia);
		} else if (ptr->ai_family == AF_INET6) {
			sockaddr_in6 *sin6 = reinterpret_cast<sockaddr_in6 *>(ptr->ai_addr);
			internet_address ia { sin6->sin6_addr };
			ia_vec.push_back(ia);
		}
	}

	freeaddrinfo(res);
	return ia_vec;
}

std::string
get_canonical_name(const std::string &addrstr)
{
	struct addrinfo *res = nullptr;
	struct addrinfo *ptr = nullptr;
	struct addrinfo hints;
	std::string hostname;

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	get_address_info(addrstr.c_str(), nullptr, &hints, &res);

	for (ptr = res; ptr != nullptr; ptr = ptr->ai_next) {
		if (ptr->ai_canonname) {
			hostname = ptr->ai_canonname;
			break;
		}
	}

	freeaddrinfo(res);
	return hostname;
}

} // namespace net
} // namespace snf
