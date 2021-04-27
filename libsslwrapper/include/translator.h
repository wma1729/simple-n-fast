#ifndef _SNF_TRANSLATER_H_
#define _SNF_TRANSLATER_H_

#include "safestr.h"
#include <string>

namespace snf {
namespace ssl {

class translator {
public:
    static safestr *hex2bin(const std::string &);
    static std::string bin2hex(const safestr *);
    static std::string bin2hex(const uint8_t *, size_t);

    static safestr *base642bin(const std::string &);
    static std::string bin2base64(const safestr *);
    static std::string bin2base64(const uint8_t *, size_t);
};

} // namespace ssl
} // namespace snf

#endif // _SNF_TRANSLATER_H_