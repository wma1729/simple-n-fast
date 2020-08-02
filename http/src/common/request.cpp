#include "request.h"
#include "scanner.h"
#include "status.h"

namespace snf {
namespace http {

void
request::validate()
{
	std::ostringstream oss;

	if (m_headers.is_set(HOST)) {
		in_port_t user_spec_port = 0;
		const std::string &user_spec_host = m_headers.host(&user_spec_port);

		if (m_uri.get_host().is_present()) {
			if (user_spec_host != m_uri.get_host().get()) {
				oss << "host in headers " << user_spec_host
					<< " does not match host in URI " << m_uri;
				throw bad_message(oss.str());
			}
		}			

		if (user_spec_port) {
			if (m_uri.get_port().is_present()) {
				if (user_spec_port != m_uri.get_port().numeric_port()) {
					oss << "port in headers " << user_spec_port
						<< " does not match port in URI " << m_uri;
					throw bad_message(oss.str());
				}
			}
		} else {
			if (m_uri.get_port().is_present()) {
				oss << "port not specified in headers but available in URI " << m_uri;
				throw bad_message(oss.str());
			}
		}
	} else {
		throw bad_message("HTTP host field missing");
	}

	message::validate();
}

request_builder::request_builder(std::istream &is, bool ignore_body)
{
	std::string line;
	headers     hdrs;

	if (!readline(is, line))
		throw std::runtime_error("unable to read request line");

	request_line(line);

	while (true) {
		line.clear();

		if (!readline(is, line))
			throw std::runtime_error("unable to read request header");

		if (line.empty() || (line == "\r\n"))
			break;

		hdrs.add(line);
	}

	if (hdrs.empty())
		throw bad_message("a request with no header is invalid");

	with_headers(std::move(hdrs));

	if (!ignore_body) {
		body *b = nullptr;

		if (m_request.get_headers().is_message_chunked()) {
			b = body_factory::instance().from_istream(is);
		} else {
			size_t len = m_request.get_headers().content_length();
			if (len != 0)
				b = body_factory::instance().from_istream(is, len);
		}

		if (b != nullptr)
			m_request.set_body(b);
	}
}

request_builder::request_builder(snf::net::nio *io, bool ignore_body)
{
	int         syserr = 0;
	std::string line;
	headers     hdrs;

	if (io->readline(line, 1000, &syserr) != E_ok)
		throw std::system_error(
			syserr,
			std::system_category(),
			"unable to read request line");

	request_line(line);

	while (true) {
		line.clear();

		if (io->readline(line, 1000, &syserr) != E_ok)
				throw std::system_error(
					syserr,
					std::system_category(),
					"unable to read request header");

		if (line.empty() || (line == "\r\n"))
			break;

		hdrs.add(line);
	}

	if (hdrs.empty())
		throw bad_message("a request with no header is invalid");

	with_headers(std::move(hdrs));

	if (!ignore_body) {
		body *b = nullptr;

		if (m_request.get_headers().is_message_chunked()) {
			b = body_factory::instance().from_socket(io);
		} else {
			size_t len = m_request.get_headers().content_length();
			if (len != 0)
				b = body_factory::instance().from_socket(io, len);
		}

		if (b != nullptr)
			m_request.set_body(b);
	}
}

/*
 * Builds the request from the request line. The request
 * line has the following format: <method> <uri> <version>
 *
 * @param [in] str - the request line.
 *
 * @throws snf::http::bad_message if the request line could
 *         not be parsed.
 */
request_builder &
request_builder::request_line(const std::string &istr)
{
	std::string mstr;
	std::string ustr;
	std::string vstr;
	std::ostringstream oss;
	scanner scn{istr};

	if (!scn.read_token(mstr, false)) {
		throw bad_message("HTTP method not found");
	}

	if (!scn.read_space()) {
		oss << "no space after (" << mstr << ")";
		throw bad_message(oss.str());
	}

	if (!scn.read_uri(ustr)) {
		throw bad_message("URI not found");
	}

	if (!scn.read_space()) {
		oss << "no space after (" << mstr << " " << ustr << ")";
		throw bad_message(oss.str());
	}

	if (!scn.read_version(vstr)) {
		throw bad_message("HTTP version not found");
	}

	scn.read_opt_space();

	if (!scn.read_crlf()) {
		oss << "HTTP message (" << mstr << " " << ustr << " " << vstr << ") not terminated with CRLF";
		throw bad_message(oss.str());
	}

	m_request.m_type = snf::http::method(mstr);
	m_request.m_uri = std::move(snf::http::uri(ustr));
	m_request.m_version = snf::http::version(vstr);

	if (m_request.m_version.m_protocol != http_protocol) {
		if (m_request.m_version.m_protocol.empty())
			oss << "HTTP protocol cannot be empty";
		else
			oss << "invalid protocol " << m_request.m_version.m_protocol;
		throw bad_message(oss.str());
	}

	if ((m_request.m_version.m_major != MAJOR_VERSION) ||
		(m_request.m_version.m_minor != MINOR_VERSION)) {
		oss << "HTTP version " << m_request.m_version << " is not supported";
		throw exception(oss.str(), status_code::HTTP_VERSION_NOT_SUPPORTED);
	}

	return *this;
}

} // namespace http
} // namespace snf
