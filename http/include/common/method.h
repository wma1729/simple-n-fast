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

method_type method(const std::string &);
std::string method(method_type) noexcept;

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_METHOD_H_
