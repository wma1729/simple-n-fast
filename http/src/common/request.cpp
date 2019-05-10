#include "request.h"
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
		throw std::runtime_error("empty method");

	if (i >= len) {
		oss << "no uri after method (" << mstr << ")";
		throw std::runtime_error(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after method (" << mstr << ")";
		throw std::runtime_error(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		ustr.push_back(istr[i]);
	}

	if (ustr.empty())
		throw std::runtime_error("empty URI");

	if (i >= len) {
		oss << "no version after " << mstr << " " << ustr;
		throw std::runtime_error(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after " << mstr << " " << ustr;
		throw std::runtime_error(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (is_whitespace(istr[i]))
			break;
		vstr.push_back(istr[i]);
	}

	if (vstr.empty())
		throw std::runtime_error("empty version");

	if (is_whitespace(istr[i])) {
		oss << "unexpected space found after " << mstr << " " << ustr << " " << vstr;
		throw std::runtime_error(oss.str());
	}

	m_request.m_type = snf::http::method(mstr);
	m_request.m_uri = std::move(snf::http::uri(vstr));
	m_request.m_version = snf::http::version(vstr);

	return *this;
}

request_builder &
request_builder::with_headers(const headers &hdrs)
{
	m_request.m_headers = hdrs;
	return *this;
}

request_builder &
request_builder::with_headers(headers &&hdrs)
{
	m_request.m_headers = std::move(hdrs);
	return *this;
}

void
request_builder::validate()
{
	int64_t body_length = m_request.m_body ? m_request.m_body->length() : 0;
	int64_t mesg_length = m_request.m_headers.is_set(CONTENT_LENGTH) ?
		m_request.m_headers.content_length() : 0;

	bool body_chunked = m_request.m_body ? m_request.m_body->chunked() : false;
	bool mesg_chunked = m_request.m_headers.is_message_chunked();

	// trust body info over message info

	if (body_length != mesg_length) {
		m_request.m_headers.content_length(body_length);
	}

	if (body_chunked) {
		if (!mesg_chunked) {
			m_request.m_headers.transfer_encoding(TRANSFER_ENCODING_CHUNKED);
		}
	} else {
		if (mesg_chunked) {
			m_request.m_headers.remove(TRANSFER_ENCODING);
		}
	}

	// make sure everything is fine

	bool error = false;
	std::ostringstream oss;

	if (body_chunked) {
		if (!m_request.m_headers.is_set(TRANSFER_ENCODING)) {
			error = true;
			oss << TRANSFER_ENCODING << " header is not set";
		} else if (TRANSFER_ENCODING_CHUNKED != m_request.m_headers.transfer_encoding()) {
			error = true;
			oss << TRANSFER_ENCODING << " header is not set to " << TRANSFER_ENCODING_CHUNKED;
		}
	}

	if (!error) {
		if (body_length) {
			if (!m_request.m_headers.is_set(CONTENT_LENGTH)) {
				error = true;
				oss << CONTENT_LENGTH << " header is not set";
			} else if (body_length != m_request.m_headers.content_length()) {
				error = true;
				oss << CONTENT_LENGTH
					<< " header is incorrectly set to "
					<< m_request.m_headers.content_length()
					<< "; the correct value is " << body_length;
			}
		}
	}

	if (error) {
		throw std::runtime_error(oss.str());
	}
}

} // namespace http
} // namespace snf
