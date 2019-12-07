#include "handler.h"
#include "server.h"
#include "logmgr.h"
#include "uri.h"
#include "transmit.h"
#include "status.h"
#include "router.h"
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

			sock_t thesock = *sock;
			server::instance().reactor().add_handler(
				thesock,
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
	std::unique_ptr<snf::net::socket> sptr(s);
	bool close_connection = false;

	transmitter xfer(ioptr.get());

	int retval = E_ok;
	status_code status = status_code::OK;
	std::string errmsg;

	try {
		request req(std::move(xfer.recv_request()));
		DEBUG_STRM(nullptr)
			<< "client request: "
			<< method(req.get_method())
			<< " " << req.get_uri()
			<< snf::log::record::endl;

		response resp(std::move(router::instance().handle(req)));		

		status = resp.get_status();

		retval = xfer.send_response(resp);

		const std::vector<std::string> &req_conn = req.get_headers().connection();
		for (auto s : req_conn) {
			if (s == CONNECTION_CLOSE) {
				close_connection = true;
				break;
			}
		}

		if (!close_connection) {
			const std::vector<std::string> &resp_conn = resp.get_headers().connection();
			for (auto s : resp_conn) {
				if (s == CONNECTION_CLOSE) {
					close_connection = true;
					break;
				}
			}
		}
	} catch (const bad_message &ex) {
		status = status_code::BAD_REQUEST;
		errmsg = ex.what();
	} catch (const bad_uri &ex) {
		status = status_code::BAD_REQUEST;
		errmsg = ex.what();
	} catch (const not_found &ex) {
		status = status_code::NOT_FOUND;
		errmsg = ex.what();
	} catch (const not_implemented &ex) {
		status = status_code::NOT_IMPLEMENTED;
		errmsg = ex.what();
	} catch (const snf::http::exception &ex) {
		status = ex.get_status_code();
		errmsg = ex.what();
	} catch (const std::system_error &ex) {
		status = status_code::INTERNAL_SERVER_ERROR;
		errmsg = ex.what();
	} catch (const std::runtime_error &ex) {
		status = status_code::INTERNAL_SERVER_ERROR;
		errmsg = ex.what();
	}

	if (is_http_error(status)) {
		response resp(gen_error_resp(status, errmsg.c_str()));
		retval = xfer.send_response(resp);
	}

	DEBUG_STRM(nullptr)
		<< "server response: "
		<< status
		<< snf::log::record::endl;

	if (is_http_error(status) || (E_ok != retval) || close_connection) {
		if (sptr) {
			snf::net::ssl::connection *cnxn = dynamic_cast<snf::net::ssl::connection *>(ioptr.get());
			if (cnxn)
				cnxn->shutdown();
		}
	} else {
		sock_t thesock = *s;
		server::instance().reactor().add_handler(
			thesock,
			snf::net::event::read,
			DBG_NEW read_handler(ioptr.release(), sptr.get(), snf::net::event::read));
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
