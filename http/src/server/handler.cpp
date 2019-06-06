#include "handler.h"
#include "server.h"
#include "logmgr.h"
#include "uri.h"
#include "transmit.h"
#include "status.h"
#include <ostream>
#include <sstream>
#include <thread>

namespace snf {
namespace http {

void
process_ssl_handshake(snf::net::socket *s)
{
	std::unique_ptr<snf::net::socket> sock(s);

	try {
		std::unique_ptr<snf::net::ssl::connection> cnxn(
			DBG_NEW snf::net::ssl::connection(
				snf::net::connection_mode::server,
				server::instance().ssl_context())
			);
		cnxn->handshake(*sock);
		std::string errstr;
		if (cnxn->is_verification_successful(errstr)) {
			DEBUG_STRM(nullptr)
				<< "SSL handshake successful for socket "
				<< *sock
				<< snf::log::record::endl;
			server::instance().reactor().add_handler(
				*sock,
				snf::net::event::read,
				DBG_NEW read_handler(cnxn.release(), sock.release(), snf::net::event::read));
		} else {
			ERROR_STRM(nullptr)
				<< "SSL handshake failed for socket "
				<< *sock
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
	if (*m_sock != s) {
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
		return false;
	}

	try {
		snf::net::socket *nsock = DBG_NEW snf::net::socket(std::move(m_sock->accept()));
		INFO_STRM("accept_handler")
			<< "accepted socket "
			<< *nsock
			<< snf::log::record::endl;

		if (is_secured()) {
			server::instance().thread_pool()->submit(process_ssl_handshake, nsock);
		} else {
			server::instance().reactor().add_handler(
					*nsock,
					snf::net::event::read,
					DBG_NEW read_handler(nsock, snf::net::event::read));
		}
		return true;
	} catch (std::system_error &ex) {
		ERROR_STRM("accept_handler", ex.code().value())
			<< ex.what()
			<< snf::log::record::endl;
		return false;
	}
}

void
process_request(snf::net::nio *io, snf::net::socket *s)
{
	std::unique_ptr<snf::net::nio> ioptr(io);
	std::unique_ptr<snf::net::socket> sockptr(s);

	transmitter xfer(ioptr.get());
	try {
		request req = std::move(xfer.recv_request());
	} catch (bad_message &) {
	} catch (bad_uri &) {
	} catch (not_implemented &) {
	} catch (std::system_error &) {
	} catch (std::runtime_error &) {
	}
}

bool
read_handler::operator()(sock_t s, snf::net::event e)
{
	snf::net::socket *sock = nullptr;

	if (m_sock)
		sock = dynamic_cast<snf::net::socket *>(m_sock.get());
	else
		sock = dynamic_cast<snf::net::socket *>(m_io.get());

	if (sock == nullptr) {
		ERROR_STRM("read_handler")
			<< "invalid socket"
			<< snf::log::record::endl;
		return false;
	}

	if (*sock != s) {
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
		sock->close();
		return false;
	}

	server::instance().thread_pool()->submit(process_request, m_io.release(), m_sock.release());

	// Do not register it again.
	return false;
}

} // namespace http
} // namespace snf
