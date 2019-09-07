#include "message.h"
#include "status.h"

namespace snf {
namespace http {

/*
 * Validates if
 * - Content-Length is appropriately set if the message is
 *   not chunked.
 * - Transfer-Encoding is set correctly to chunked if the
 *   message is chunked.
 *
 * @throws snf::http::bad_message if the Content-Length/
 *         Transfer-Encoding is not set correctly.
 */
void
message::validate_length_chunkiness()
{
	// trust body info over message info

	if (m_body) {
		bool body_chunked = m_body->chunked();
		bool mesg_chunked = m_headers.is_message_chunked();
		if (body_chunked != mesg_chunked) {
			if (body_chunked)
				m_headers.transfer_encoding(TRANSFER_ENCODING_CHUNKED);
			else
				m_headers.remove(TRANSFER_ENCODING);
		}

		size_t body_length = m_body->length();
		size_t mesg_length = m_headers.is_set(CONTENT_LENGTH) ?
					m_headers.content_length() : 0;
		if (body_length != mesg_length)
			m_headers.content_length(body_length);
	}

	// make sure everything is fine

	if (m_headers.is_set(TRANSFER_ENCODING)) {
		if (TRANSFER_ENCODING_CHUNKED != m_headers.transfer_encoding()) {
			std::ostringstream oss;
			oss << TRANSFER_ENCODING << " header is not set to " << TRANSFER_ENCODING_CHUNKED;
			throw bad_message(oss.str());
		}

		if (m_headers.is_set(CONTENT_LENGTH)) {
			if (0 != m_headers.content_length()) {
				m_headers.content_length(0);
			}
		}
	}
}

/*
 * Validates if the message is formulated correctly.
 * Things validated:
 * 1) Content-Length and/or Transfer-Encoding set appropriately.
 * 2) ...
 *
 * @throws snf::http::bad_message if the message is malformed.
 */
void
message::validate()
{
	validate_length_chunkiness();
}

} // namespace http
} // namespace snf
