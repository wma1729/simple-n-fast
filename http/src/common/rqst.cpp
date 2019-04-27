#include "rqst.h"
#include "status.h"
#include "charset.h"

namespace snf {
namespace http {

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
		throw http_exception("empty method", status_code::BAD_REQUEST);

	if (i >= len) {
		oss << "no uri after method (" << mstr << ")";
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	if (istr[i] != ' ') {
		oss << "no space after method (" << mstr << ")";
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		ustr.push_back(istr[i]);
	}

	if (ustr.empty())
		throw http_exception("empty URI", status_code::BAD_REQUEST);

	if (i >= len) {
		oss << "no version after " << mstr << " " << ustr;
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	if (istr[i] != ' ') {
		oss << "no space after " << mstr << " " << ustr;
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	for (i = i + 1; i < len; ++i) {
		if (is_whitespace(istr[i]))
			break;
		vstr.push_back(istr[i]);
	}

	if (vstr.empty())
		throw http_exception("empty version", status_code::BAD_REQUEST);

	if (is_whitespace(istr[i])) {
		oss << "unexpected space found after " << mstr << " " << ustr << " " << vstr;
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	m_request.m_type = snf::http::method(mstr);
	m_request.m_uri = std::move(snf::http::uri(vstr));
	m_request.m_version = snf::http::version(vstr);

	return *this;
}

request_builder &
request_builder::with_header(const std::string &name, const std::string &value)
{
	m_request.m_headers.add(name, value);
	return *this;
}

request_builder &
request_builder::header_line(const std::string &istr)
{
	m_request.m_headers.add(istr);
	return *this;
}

} // namespace http
} // namespace snf
