#include "response.h"
#include "scanner.h"
#include "charset.h"

namespace snf {
namespace http {

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
	m_response.m_status = static_cast<status_code>(std::stoi(sstr));
	m_response.m_reason = rstr;

	return *this;
}

} // namespace http
} // namespace snf
