#include "message.h"

namespace snf {
namespace http {

void
message::validate()
{
	bool trust_body = false;

	size_t body_length = 0;
	bool body_chunked = false;

	if (m_body) {
		body_length = m_body->length();
		body_chunked = m_body->chunked();
		trust_body = true;
	}

	size_t mesg_length = m_headers.is_set(CONTENT_LENGTH) ?
				m_headers.content_length() : 0;
	bool mesg_chunked = m_headers.is_message_chunked();


	// trust body info over message info

	if (body_length != mesg_length) {
		if (trust_body)
			m_headers.content_length(body_length);
	}

	if (body_chunked) {
		if (!mesg_chunked) {
			m_headers.transfer_encoding(TRANSFER_ENCODING_CHUNKED);
		}
	} else if (trust_body) {
		if (mesg_chunked) {
			m_headers.remove(TRANSFER_ENCODING);
		}
	}

	// make sure everything is fine

	bool error = false;
	std::ostringstream oss;

	if (body_chunked) {
		if (!m_headers.is_set(TRANSFER_ENCODING)) {
			error = true;
			oss << TRANSFER_ENCODING << " header is not set";
		} else if (TRANSFER_ENCODING_CHUNKED != m_headers.transfer_encoding()) {
			error = true;
			oss << TRANSFER_ENCODING << " header is not set to " << TRANSFER_ENCODING_CHUNKED;
		}
	}

	if (!error) {
		if (body_length) {
			if (!m_headers.is_set(CONTENT_LENGTH)) {
				error = true;
				oss << CONTENT_LENGTH << " header is not set";
			} else if (body_length != m_headers.content_length()) {
				error = true;
				oss << CONTENT_LENGTH
					<< " header is incorrectly set to "
					<< m_headers.content_length()
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
