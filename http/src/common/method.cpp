#include "method.h"
#include <stdexcept>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

method_type
method(const std::string &str)
{
	if (str == "GET")
		return method_type::GET;
	else if (str == "HEAD")
		return method_type::HEAD;
	else if (str == "POST")
		return method_type::POST;
	else if (str == "PUT")
		return method_type::PUT;
	else if (str == "DELETE")
		return method_type::DELETE;
	else if (str == "CONNECT")
		return method_type::CONNECT;
	else if (str == "OPTIONS")
		return method_type::OPTIONS;
	else if (str == "TRACE")
		return method_type::TRACE;

	std::ostringstream oss;
	oss << "invalid HTTP method (" << str << ")";
	throw std::runtime_error(oss.str());
}

} // namespace http
} // namespace snf
