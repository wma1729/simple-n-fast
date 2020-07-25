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
	if (m_body) {
		if (m_body->chunked()) {
			// make sure there is no Content-Length
			m_headers.remove(CONTENT_LENGTH);

			// add chunked transfer encoding if not already set
			if (m_headers.is_set(TRANSFER_ENCODING)) {
				std::vector<token> encodings = m_headers.transfer_encoding();
				std::vector<token>::const_iterator it =
					std::find(encodings.begin(), encodings.end(), TRANSFER_ENCODING_CHUNKED);
				if (it == encodings.end()) {
					encodings.push_back(TRANSFER_ENCODING_CHUNKED);
					m_headers.transfer_encoding(encodings);
				}
			} else {
				m_headers.transfer_encoding(TRANSFER_ENCODING_CHUNKED);
			}
		} else {
			// make sure chunked transfer encoding is not set
			if (m_headers.is_set(TRANSFER_ENCODING)) {
				std::vector<token> encodings = m_headers.transfer_encoding();
				std::vector<token>::const_iterator it =
					std::find(encodings.begin(), encodings.end(), TRANSFER_ENCODING_CHUNKED);
				if (it != encodings.end()) {
					encodings.erase(it);
					if (encodings.empty())
						m_headers.remove(TRANSFER_ENCODING);
					else
						m_headers.transfer_encoding(encodings);
				}
			}

			// add Content-Length
			m_headers.content_length(m_body->length());
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
