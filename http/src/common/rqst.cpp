#include "rqst.h"
#include "status.h"

namespace snf {
namespace http {

request_builder &
request_builder::request_line(const std::string &istr)
{
	size_t i;
	std::string str;
	size_t len = istr.size();

	if ((istr[len - 1] != '\n') || (istr[len - 2] != '\r'))
		throw http_exception("not properly terminated", status_code::BAD_REQUEST);

	len -= 2;

	for (i = 0; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		str.push_back(istr[i]);
	}

	if (i >= len)
		throw http_exception("no uri after method", status_code::BAD_REQUEST);

	if (istr[i] != ' ')
		throw http_exception("no space after method", status_code::BAD_REQUEST);

	if (str.empty())
		throw http_exception("empty method", status_code::BAD_REQUEST);

	m_request.m_type = snf::http::method(str);
	str.clear();

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		str.push_back(istr[i]);
	}

	if (i >= len)
		throw http_exception("no version after URI", status_code::BAD_REQUEST);

	if (istr[i] != ' ')
		throw http_exception("no space after URI", status_code::BAD_REQUEST);

	if (str.empty())
		throw http_exception("empty URI", status_code::BAD_REQUEST);

	m_request.m_uri = std::move(snf::http::uri(str));
	str.clear();

	for (i = i + 1; i < len; ++i) {
		if (std::isspace(istr[i]))
			break;
		str.push_back(istr[i]);
	}

	if (std::isspace(istr[i]))
		throw http_exception("unexpected space found", status_code::BAD_REQUEST);

	if (str.empty())
		throw http_exception("empty version", status_code::BAD_REQUEST);

	m_request.m_version = snf::http::version(str);

	return *this;
}

} // namespace http
} // namespace snf
