#ifndef _SNF_HTTP_CMN_PARSEUTIL_H_
#define _SNF_HTTP_CMN_PARSEUTIL_H_

#include <string>
#include <vector>
#include <utility>

namespace snf {
namespace http {

using param_vec_t = std::vector<std::pair<std::string, std::string>>;

std::string parse_token(const std::string &, size_t &, size_t);
param_vec_t parse_parameter(const std::string &, size_t &, size_t);
std::vector<std::string> parse_list(const std::string &);
std::string parse_generic(const std::string &, size_t &, size_t);

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_PARSEUTIL_H_
