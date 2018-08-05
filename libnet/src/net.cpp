#include "net.h"
#include "logmgr.h"

namespace snf {
namespace net {

bool
initialize()
{
#if defined(_WIN32)
	WSADATA wsa_data;

	int status = WSAStartup(MAKEWORD(2, 2), &wsa_data);
	if (status != 0) {
		ERROR_STRM(nullptr, status)
			<< "failed to initialize Winsock DLL"
			<< snf::log::record::endl;
		return false;
	} else if ((LOBYTE(wsa_data.wVersion) != 2) || (HIBYTE(wsa_data.wVersion) != 2)) {
		DEBUG_STRM(nullptr)
			<< "failed to find appropriate Winsock DLL version"
			<< snf::log::record::endl;
		WSACleanup();
		return false;
	} else {
		std::atexit(snf::net::finalize);
	}
#endif
	return true;
}

void
finalize()
{
#if defined(_WIN32)
	WSACleanup();
#endif
}

} // namespace net
} // namespace snf
