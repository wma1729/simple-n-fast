#include "sslfcn.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace snf {
namespace net {
namespace ssl {

ssl_library::ssl_library()
{
	const char *libname = "libssl.so";
	const char *env = getenv("LIBSSL");
	if (env && *env)
		libname = env;
	ssl = new snf::dll(libname);
}

ssl_library::~ssl_library()
{
	if (ssl)
		delete ssl;
}

p_library_init
ssl_library::get_library_init()
{
	if (!library_init)
		library_init = reinterpret_cast<p_library_init>
			(ssl->symbol("SSL_library_init"));
	return library_init;
}

p_load_error_strings
ssl_library::get_load_error_strings()
{
	if (!load_error_strings)
		load_error_strings = reinterpret_cast<p_load_error_strings>
			(ssl->symbol("SSL_load_error_strings"));
	return load_error_strings;
}

p_free_error_strings
ssl_library::get_free_error_strings()
{
	if (!free_error_strings)
		free_error_strings = reinterpret_cast<p_free_error_strings>
			(ssl->symbol("ERR_free_strings"));
	return free_error_strings;
}

} // namespace ssl
} // namespace net
} // namespace snf
