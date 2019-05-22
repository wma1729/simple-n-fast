#include "handler.h"
#include "server.h"
#include "logmgr.h"
#include <ostream>
#include <sstream>
#include <thread>

namespace snf {
namespace http {

void
perform_ssl_handshake(snf::net::socket &&sock)
{
	std::unique_ptr<snf::net::ssl::connection> cnxn(
		DBG_NEW snf::net::ssl::connection(
			snf::net::connection_mode::server,
			server::instance().ssl_context())
		);
	cnxn->handshake(sock);
	std::string errstr;
	if (cnxn->is_verification_successful(errstr)) {
		DEBUG_STRM(nullptr)
			<< "SSL handshake successful for socket "
			<< sock
			<< snf::log::record::endl;
	} else {
		ERROR_STRM(nullptr)
			<< "SSL handshake failed for socket "
			<< sock
			<< ": "
			<< errstr
			<< snf::log::record::endl;
	}
}

bool
accept_handler::operator()(sock_t s, snf::net::event e)
{
	if (m_sock != s) {
		ERROR_STRM("accept_handler")
			<< "socket mismatch"
			<< snf::log::record::endl;
		return false;
	}

	if (e == snf::net::event::read) {
		try {
			snf::net::socket nsock = std::move(m_sock.accept());
			INFO_STRM("accept_handler")
				<< "accepted socket "
				<< nsock
				<< snf::log::record::endl;

			if (is_secured()) {
				std::thread t(perform_ssl_handshake, std::move(nsock));
				t.detach();
			} else {
			}
		} catch (std::system_error &ex) {
			ERROR_STRM("accept_handler", ex.code().value())
				<< ex.what()
				<< snf::log::record::endl;
			return false;
		}

		return true;
	} else {
		ERROR_STRM("accept_handler")
			<< "unexpected event received "
			<< snf::net::eventstr(e)
			<< snf::log::record::endl;
		m_sock.close();
		return false;
	}
}

} // namespace http
} // namespace snf
