#include "handler.h"
#include "server.h"
#include "logmgr.h"
#include <ostream>
#include <sstream>
#include <thread>

namespace snf {
namespace http {

void ssl_handshake(snf::net::socket &s)
{
	try {
		std::unique_ptr<snf::net::ssl::connection> cnxn(
			DBG_NEW snf::net::ssl::connection(
				snf::net::connection_mode::server,
				server::instance().ssl_context())
			);
		cnxn->handshake(s);
		std::string errstr;
		if (cnxn->is_verification_successful(errstr)) {
			DEBUG_STRM(nullptr)
				<< "SSL handshake successful for socket "
				<< s
				<< snf::log::record::endl;
			server::instance().reactor().add_handler(
				s,
				snf::net::event::read,
				DBG_NEW read_handler(std::move(s), snf::net::event::read));
		} else {
			ERROR_STRM(nullptr)
				<< "SSL handshake failed for socket "
				<< s
				<< ": "
				<< errstr
				<< snf::log::record::endl;
		}
	} catch (snf::net::ssl::exception &ex) {
		ERROR_STRM(nullptr)
			<< ex.what()
			<< snf::log::record::endl;
		for (auto I = ex.begin(); I != ex.end(); ++I)
			ERROR_STRM(nullptr)
				<< *I
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

	if (e != snf::net::event::read) {
		ERROR_STRM("accept_handler")
			<< "unexpected event received "
			<< snf::net::eventstr(e)
			<< snf::log::record::endl;
		m_sock.close();
		return false;
	}

	try {
		snf::net::socket nsock = std::move(m_sock.accept());
		INFO_STRM("accept_handler")
			<< "accepted socket "
			<< nsock
			<< snf::log::record::endl;

		if (is_secured()) {
			server::instance().thread_pool()->submit(ssl_handshake, std::ref(nsock));
		} else {
			server::instance().reactor().add_handler(
					nsock,
					snf::net::event::read,
					DBG_NEW read_handler(std::move(nsock), snf::net::event::read));
		}
		return true;
	} catch (std::system_error &ex) {
		ERROR_STRM("accept_handler", ex.code().value())
			<< ex.what()
			<< snf::log::record::endl;
		return false;
	}
}

bool
read_handler::operator()(sock_t s, snf::net::event e)
{
	if (m_sock != s) {
		ERROR_STRM("read_handler")
			<< "socket mismatch"
			<< snf::log::record::endl;
		return false;
	}

	if (e != snf::net::event::read) {
		ERROR_STRM("read_handler")
			<< "unexpected event received "
			<< snf::net::eventstr(e)
			<< snf::log::record::endl;
		m_sock.close();
		return false;
	}

	// process the request here

	return true;
}

} // namespace http
} // namespace snf
