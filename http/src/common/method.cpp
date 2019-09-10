#include "method.h"
#include "status.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

/*
 * Stream operator for the method type.
 *
 * @param [out] os - output stream.
 * @param [in]  mt - method type.
 *
 * @return stream representation of the method type.
 */
std::ostream &
operator<<(std::ostream &os, method_type mtype)
{
	switch (mtype) {
		case method_type::M_GET: os << "GET"; break;
		case method_type::M_HEAD: os << "HEAD"; break;
		case method_type::M_POST: os << "POST"; break;
		case method_type::M_PUT: os << "PUT"; break;
		case method_type::M_DELETE: os << "DELETE"; break;
		case method_type::M_CONNECT: os << "CONNECT"; break;
		case method_type::M_OPTIONS: os << "OPTIONS"; break;
		case method_type::M_TRACE: os << "TRACE"; break;
		default: break;
	}
	return os;
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
	std::ostringstream oss;
	oss << mtype;
	return oss.str();
}

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

} // namespace http
} // namespace snf
