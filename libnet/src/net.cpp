#include "net.h"
#include "sslfcn.h"
#include "error.h"
#include <mutex>

namespace snf {
namespace net {

void
do_initialize()
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
	return;
}

void
initialize()
{
	static std::once_flag do_once;
	std::call_once(do_once, snf::net::do_initialize);
}

void
finalize()
{
#if defined(_WIN32)
	WSACleanup();
#endif
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

/*
 * Poll sockets for events. Look at poll(2) for more details.
 *
 * @param [inout] fds   - vector of pollfd elements.
 * @param [in]    to    - timeout in milliseconds.
 *                        POLL_WAIT_FOREVER for inifinite wait.
 *                        POLL_WAIT_NONE for no wait.
 * @param [out]   oserr - system error in case of failure, if not null.
 *
 * @return >0 indicating the number of sockets that are ready.
 *         =0 if the call times out before any socket is ready.
 *         <0 in case of error.
 */
int
poll(std::vector<pollfd> &fds, int to, int *oserr)
{
	int retval;

	if (oserr)
		*oserr = 0;

	do {
#if defined(_WIN32)
		retval = WSAPoll(fds.data(), static_cast<ULONG>(fds.size()), to);
#else
		retval = poll(fds.data(), fds.size(), to);
#endif
		if (retval == SOCKET_ERROR) {
			int error = snf::net::error();
#if !defined(_WIN32)
			if (EINTR == error) continue;
#endif
			if (oserr) *oserr = error;
		}

		break;
	} while (true);

	return retval;
}

} // namespace net
} // namespace snf
