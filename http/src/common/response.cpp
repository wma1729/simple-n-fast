#include "common.h"
#include "response.h"
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
	size_t i;
	std::string vstr;
	std::string sstr;
	std::string rstr;
	size_t len = istr.size();
	std::ostringstream oss;

	if ((istr[len - 1] == '\n') || (istr[len - 2] == '\r'))
		len -= 2;

	for (i = 0; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		vstr.push_back(istr[i]);
	}

	if (vstr.empty())
		throw bad_message("empty version");

	if (i >= len) {
		oss << "no status code after version (" << vstr << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after version (" << vstr << ")";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		if (!std::isdigit(istr[i])) {
			oss << "unexpected character (" << istr[i] << ") in status code";
			throw bad_message("empty status code");
		}

		sstr.push_back(istr[i]);
	}

	if (sstr.empty())
		throw bad_message("empty status code");

	if (i >= len) {
		oss << "no reason phrase after version/status ("
			<< vstr << " " << sstr << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after version/status ("
			<< vstr << " " << sstr << ")";
		throw bad_message(oss.str());
	}

	if (sstr.size() != 3) {
		oss << "invalid status code (" << sstr << ")";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (is_whitespace(istr[i]) || is_vchar(istr[i]) || is_opaque(istr[i]))
			rstr.push_back(istr[i]);
		else
			break;
	}

	if (rstr.empty())
		throw bad_message("empty reason phrase");

	if (i != len)
		throw bad_message("unexpected character found in reason phrase");

	m_response.m_version = snf::http::version(vstr);
	m_response.m_status = static_cast<status_code>(std::stoi(sstr));
	m_response.m_reason = std::move(snf::trim(rstr));

	return *this;
}

} // namespace http
} // namespace snf
