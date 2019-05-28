#include "request.h"
#include "charset.h"
#include "status.h"

namespace snf {
namespace http {

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
	size_t i;
	std::string mstr;
	std::string ustr;
	std::string vstr;
	size_t len = istr.size();
	std::ostringstream oss;

	if ((istr[len - 1] == '\n') || (istr[len - 2] == '\r'))
		len -= 2;

	for (i = 0; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		mstr.push_back(istr[i]);
	}

	if (mstr.empty())
		throw bad_message("empty method");

	if (i >= len) {
		oss << "no uri after method (" << mstr << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after method (" << mstr << ")";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		ustr.push_back(istr[i]);
	}

	if (ustr.empty())
		throw bad_message("empty URI");

	if (i >= len) {
		oss << "no version after method/URI ("
			<< mstr << " " << ustr << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after method/URI ("
			<< mstr << " " << ustr << ")";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (is_whitespace(istr[i]))
			break;
		vstr.push_back(istr[i]);
	}

	if (vstr.empty())
		throw bad_message("empty version");

	if (is_whitespace(istr[i])) {
		oss << "unexpected space found after method/URI/version ("
			<< mstr << " " << ustr << " " << vstr << ")";
		throw bad_message(oss.str());
	}

	m_request.m_type = snf::http::method(mstr);
	m_request.m_uri = std::move(snf::http::uri(vstr));
	m_request.m_version = snf::http::version(vstr);

	return *this;
}

} // namespace http
} // namespace snf
