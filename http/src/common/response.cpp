#include "response.h"
#include "scanner.h"
#include "charset.h"

namespace snf {
namespace http {

response_builder::response_builder(std::istream &is, bool ignore_body)
{
	std::string line;
	headers     hdrs;

	if (!readline(is, line))
		throw std::runtime_error("unable to read response line");

	response_line(line);

	while (true) {
		line.clear();

		if (!readline(is, line))
			throw std::runtime_error("unable to read response header");

		if (line.empty() || (line == "\r\n"))
			break;

		hdrs.add(line);
	}

	if (hdrs.empty())
		throw bad_message("a response with no header is invalid");

	with_headers(std::move(hdrs));

	if (!ignore_body) {
		body *b = nullptr;

		if (m_response.get_headers().is_message_chunked()) {
			b = body_factory::instance().from_istream(is);
		} else {
			size_t len = m_response.get_headers().content_length();
			if (len != 0)
				b = body_factory::instance().from_istream(is, len);
		}

		if (b != nullptr)
			m_response.set_body(b);
	}
}

response_builder::response_builder(snf::net::nio *io, bool ignore_body)
{
	int         syserr = 0;
	std::string line;
	headers     hdrs;

	if (io->readline(line, 1000, &syserr) != E_ok)
		throw std::system_error(
			syserr,
			std::system_category(),
			"unable to read response line");

	response_line(line);

	while (true) {
		line.clear();

		if (io->readline(line, 1000, &syserr) != E_ok)
			throw std::system_error(
				syserr,
				std::system_category(),
				"unable to read response header");

		if (line.empty() || (line == "\r\n"))
			break;

		hdrs.add(line);
	}

	if (hdrs.empty())
		throw bad_message("a response with no header is invalid");

	with_headers(std::move(hdrs));

	if (!ignore_body) {
		body *b = nullptr;

		if (m_response.get_headers().is_message_chunked()) {
			b = body_factory::instance().from_socket(io);
		} else {
			size_t len = m_response.get_headers().content_length();
			if (len != 0)
				b = body_factory::instance().from_socket(io, len);
		}

		if (b != nullptr)
			m_response.set_body(b);
	}
}

/*
 * Builds the response from the response/status line. The
 * response line has the following format: <version> <status> <reason>
 *
 * @param [in] str - the response line.
 *
 * @throws snf::http::bad_message if the response line could
 *         not be parsed.
 */
response_builder &
response_builder::response_line(const std::string &istr)
{
	std::string vstr;
	std::string sstr;
	std::string rstr;
	std::ostringstream oss;
	scanner scn{istr};

	if (!scn.read_version(vstr)) {
		throw bad_message("HTTP version not found");
	}

	if (!scn.read_space()) {
		oss << "no space after (" << vstr << ")";
		throw bad_message(oss.str());
	}

	if (!scn.read_status(sstr)) {
		throw bad_message("HTTP status not found");
	}

	if (!scn.read_space()) {
		oss << "no space after (" << vstr << " " << sstr << ")";
		throw bad_message(oss.str());
	}

	if (!scn.read_reason(rstr)) {
		throw bad_message("HTTP reason phrase not found");
	}

	scn.read_opt_space();

	if (!scn.read_crlf()) {
		oss << "HTTP message (" << vstr << " " << sstr << " " << rstr << ") not terminated with CRLF";
		throw bad_message(oss.str());
	}

	m_response.m_version = snf::http::version(vstr);

	if (m_response.m_version.m_protocol != http_protocol) {
		if (m_response.m_version.m_protocol.empty())
			oss << "HTTP protocol cannot be empty";
		else
			oss << "invalid protocol " << m_response.m_version.m_protocol;
		throw bad_message(oss.str());
	}

	if ((m_response.m_version.m_major != MAJOR_VERSION) ||
		(m_response.m_version.m_minor != MINOR_VERSION)) {
		oss << "HTTP version " << m_response.m_version << " is not supported";
		throw exception(oss.str(), status_code::HTTP_VERSION_NOT_SUPPORTED);
	}

	m_response.m_status = static_cast<status_code>(std::stoi(sstr));
	m_response.m_reason = rstr;

	return *this;
}

} // namespace http
} // namespace snf
