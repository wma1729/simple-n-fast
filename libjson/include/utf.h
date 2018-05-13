#ifndef _SNF_UTF_H_
#define _SNF_UTF_H_

#include <cstdint>
#include <string>
#include <istream>

namespace snf {
namespace json {

std::string utf16_decode(std::istream &);
std::string utf16_encode(std::istream &);

} // json
} // snf

#endif // _SNF_UTF_H_
