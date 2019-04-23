#ifndef _SNF_METHOD_H_
#define _SNF_METHOD_H_

#include <string>

namespace snf {
namespace http {

enum class method_type
{
	UNKNOWN = 0,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	CONNECT,
	OPTIONS,
	TRACE
};

method_type method(const std::string &);

} // namespace http
} // namespace snf

#endif // _SNF_METHOD_H_
