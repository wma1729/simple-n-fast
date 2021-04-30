#ifndef _SNF_CODER_H_
#define _SNF_CODER_H_

#include "safestr.h"
#include <string>

namespace snf {
namespace ssl {

class coder
{
public:
	virtual ~coder() {};
	static coder *type(const std::string &);
	virtual safestr *decode(const std::string &) = 0;
	virtual std::string encode(const safestr &) = 0;
	virtual std::string encode(const uint8_t *, size_t) = 0;
};

} // namespace ssl
} // namespace snf

#endif // _SNF_CODER_H_
