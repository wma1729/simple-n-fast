#include "method.h"
#include "status.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

/*
 * Method name to method type.
 *
 * @param [in] str - method name.
 *
 * @return method type.
 *
 * @throws snf::http::bad_message if the method name is invalid.
 */
method_type
method(const std::string &str)
{
	if (str == "GET")
		return method_type::M_GET;
	else if (str == "HEAD")
		return method_type::M_HEAD;
	else if (str == "POST")
		return method_type::M_POST;
	else if (str == "PUT")
		return method_type::M_PUT;
	else if (str == "DELETE")
		return method_type::M_DELETE;
	else if (str == "CONNECT")
		return method_type::M_CONNECT;
	else if (str == "OPTIONS")
		return method_type::M_OPTIONS;
	else if (str == "TRACE")
		return method_type::M_TRACE;

	std::ostringstream oss;
	oss << "invalid HTTP method (" << str << ")";
	throw bad_message(oss.str());
}

/*
 * Method type to method name.
 *
 * @param [in] mtype - method type.
 *
 * @return method name.
 */
std::string
method(method_type mtype) noexcept
{
	switch (mtype) {
		case method_type::M_GET:
			return "GET";

		case method_type::M_HEAD:
			return "HEAD";

		case method_type::M_POST:
			return "POST";

		case method_type::M_PUT:
			return "PUT";

		case method_type::M_DELETE:
			return "DELETE";

		case method_type::M_CONNECT:
			return "CONNECT";

		case method_type::M_OPTIONS:
			return "OPTIONS";

		case method_type::M_TRACE:
			return "TRACE";

		default:
			return std::string();
	}
}

} // namespace http
} // namespace snf
