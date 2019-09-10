#ifndef _SNF_HTTP_CMN_METHOD_H_
#define _SNF_HTTP_CMN_METHOD_H_

#include <string>

namespace snf {
namespace http {

/*
 * HTTP method.
 */
enum class method_type
{
	M_GET,
	M_HEAD,
	M_POST,
	M_PUT,
	M_DELETE,
	M_CONNECT,
	M_OPTIONS,
	M_TRACE
};

std::ostream &operator<<(std::ostream &, method_type);
std::string method(method_type) noexcept;
method_type method(const std::string &);

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_METHOD_H_
