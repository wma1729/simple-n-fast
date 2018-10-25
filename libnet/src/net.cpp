#include "net.h"
#include "sslfcn.h"
#include "error.h"
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
	std::atexit(finalize);
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

unsigned long
openssl_version(std::string &ver_str)
{
	ver_str = ssl::ssl_library::instance().openssl_version_str()(OPENSSL_VERSION);
	return ssl::ssl_library::instance().openssl_version_num()();
}

int
map_system_error(int error, int default_retval)
{
	int retval = default_retval;

#if defined(_WIN32)
	if (WSAEWOULDBLOCK == error)
		retval = E_try_again;
	else if ((WSAECONNRESET == error) || (WSAECONNABORTED == error))
		retval = E_connection_reset;
	else if (WSAETIMEDOUT == error)
		retval = E_timed_out;
#else
	if ((EAGAIN == error) || (EWOULDBLOCK == error))
		retval = E_try_again;
	else if ((ECONNRESET == error) || (ECONNABORTED == error))
		retval = E_connection_reset;
	else if (ETIMEDOUT == error)
		retval = E_timed_out;
	else if (EPIPE == error)
		retval = E_broken_pipe;
#endif

	return retval;
}

int
poll(std::vector<pollfd> &fds, int to, int *syserr)
{
	if (syserr)
		*syserr = 0;

#if defined(_WIN32)
	int retval = WSAPoll(fds.data(), static_cast<ULONG>(fds.size()), to);
#else
	int retval = poll(fds.data(), fds.size(), to);
#endif
	if (retval == SOCKET_ERROR)
		if (syserr)
			*syserr = snf::net::error();

	return retval;
}

} // namespace net
} // namespace snf
