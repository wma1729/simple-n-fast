#ifndef _SNF_METHOD_H_
#define _SNF_METHOD_H_

#include <string>

namespace snf {
namespace http {

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
std::string method(method_type);

} // namespace http
} // namespace snf

#endif // _SNF_METHOD_H_
