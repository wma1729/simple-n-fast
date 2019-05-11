#include "status.h"

namespace snf {
namespace http {

/*
 * Gets the reason phrase for the HTTP status code.
 *
 * @param [in] s - HTTP status code.
 *
 * @return reason phrase for the HTTP status code or
 * nullptr if the HTTP status code is not known.
 */
const char *
reason_phrase(status_code s) noexcept
{
	switch (s) {
		case status_code::CONTINUE:
			return "Continue";

		case status_code::SWITCHING_PROTOCOLS:
			return "Switching Protocols";

		case status_code::OK:
			return "OK";

		case status_code::CREATED:
			return "Created";

		case status_code::ACCEPTED:
			return "Accepted";

		case status_code::NON_AUTH_INFO:
			return "Non-Authoritative Information";

		case status_code::NO_CONTENT:
			return "No Content";

		case status_code::RESET_CONTENT:
			return "Reset Content";

		case status_code::PARTIAL_CONTENT:
			return "Partial Content";

		case status_code::MULTIPLE_CHOICES:
			return "Multiple Choices";

		case status_code::MOVED_PERMANENTLY:
			return "Moved Permanently";

		case status_code::FOUND:
			return "Found";

		case status_code::SEE_OTHER:
			return "See Other";

		case status_code::NOT_MODIFIED:
			return "Not Modified";

		case status_code::USE_PROXY:
			return "Use Proxy";

		case status_code::TEMP_REDIRECT:
			return "Temporary Redirect";

		case status_code::BAD_REQUEST:
			return "Bad Request";

		case status_code::UNAUTHORIZED:
			return "Unauthorized";

		case status_code::PAYMENT_REQD:
			return "Payment Required";

		case status_code::FORBIDDEN:
			return "Forbidden";

		case status_code::NOT_FOUND:
			return "Not Found";

		case status_code::METHOD_NOT_ALLOWED:
			return "Method Not Allowed";

		case status_code::NOT_ACCEPTABLE:
			return "Not Acceptable";

		case status_code::PROXY_AUTH_REQD:
			return "Proxy Authentication Required";

		case status_code::REQUEST_TIMEOUT:
			return "Request Timeout";

		case status_code::CONFLICT:
			return "Conflict";

		case status_code::GONE:
			return "Gone";

		case status_code::LENGTH_REQD:
			return "Length Required";

		case status_code::PRECOND_FAILED:
			return "Precondition Failed";

		case status_code::PAYLOAD_TOO_LARGE:
			return "Payload Too Large";

		case status_code::URI_TOO_LONG:
			return "URI Too Long";

		case status_code::UNSUPPORTED_MEDIA_TYPE:
			return "Unsupported Media Type";

		case status_code::RANGE_NOT_SATISFIABLE:
			return "Range Not Satisfiable";

		case status_code::EXPECTATION_FAILED:
			return "Expectation Failed";

		case status_code::UPGRADE_REQD:
			return "Upgrade Required";

		case status_code::INTERNAL_SERVER_ERROR:
			return "Internal Server Error";

		case status_code::NOT_IMPLEMENTED:
			return "Not Implemented";

		case status_code::BAD_GATEWAY:
			return "Bad Gateway";

		case status_code::SERVICE_UNAVAILABLE:
			return "Service Unavailable";

		case status_code::GATEWAY_TIMEOUT:
			return "Gateway Timeout";

		case status_code::HTTP_VERSION_NOT_SUPPORTED:
			return "HTTP Version Not Supported";

		default:
			return nullptr;
	}
}

} // namespace http
} // namespace snf
