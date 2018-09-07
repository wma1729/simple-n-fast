#ifndef _SNF_SSLFCN_H_
#define _SNF_SSLFCN_H_

#include "dll.h"

using p_library_init = int (*)(void);
using p_load_error_strings = void (*)(void);
using p_free_error_strings = void (*)(void);

namespace snf {
namespace net {
namespace ssl {

class ssl_library
{
private:
	snf::dll                *ssl = nullptr;
	p_library_init          library_init = nullptr;
	p_load_error_strings    load_error_strings = nullptr;
	p_free_error_strings    free_error_strings = nullptr;

	ssl_library();

public:
	static ssl_library &instance()
	{
		static ssl_library ssllib;
		return ssllib;
	}

	~ssl_library();

	p_library_init get_library_init();
	p_load_error_strings get_load_error_strings();
	p_free_error_strings get_free_error_strings();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SSLFCN_H_
