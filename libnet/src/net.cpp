#include "net.h"
#include "logmgr.h"
#include "sslfcn.h"
#include <mutex>

namespace snf {
namespace net {

static bool ssl_inited = false;

void
do_initialize(bool use_ssl)
{
#if defined(_WIN32)
	WSADATA wsa_data;

	int status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (status != 0) {
		throw std::runtime_error("failed to initialize Winsock DLL");
	} else if ((LOBYTE(wsa_data.wVersion) != 2) || (HIBYTE(wsa_data.wVersion) != 2)) {
		throw std::runtime_error("failed to find appropriate Winsock DLL version");
	}
#endif
	if (use_ssl) {
		ssl::ssl_library::instance().library_init()();
		ssl::ssl_library::instance().load_error_strings()();
		ssl_inited = true;
	}
	return;
}

void
initialize(bool use_ssl)
{
	static std::once_flag do_once;
	std::call_once(do_once, snf::net::do_initialize, use_ssl);
}

void
finalize()
{
	if (ssl_inited) {
		ssl::ssl_library::instance().free_error_strings()();
	}
#if defined(_WIN32)
	WSACleanup();
#endif
}

} // namespace net
} // namespace snf
