#include "sslfcn.h"
#include "dbg.h"

namespace snf {
namespace ssl {

void
exception::init()
{
	unsigned long code;
	int flags = 0, line = 0;
	const char *file = nullptr, *data = nullptr;
	const char *ptr;

	p_err_line_data err_line_data = CRYPTO_FCN<p_err_line_data>("ERR_get_error_line_data");
	p_err_lib_string err_lib_string = CRYPTO_FCN<p_err_lib_string>("ERR_lib_error_string");
	p_err_fcn_string err_fcn_string = CRYPTO_FCN<p_err_fcn_string>("ERR_func_error_string");
	p_err_reason_string err_reason_string = CRYPTO_FCN<p_err_reason_string>("ERR_reason_error_string");
	p_err_clear err_clear = CRYPTO_FCN<p_err_clear>("ERR_clear_error");


	while ((code = err_line_data(&file, &line, &data, &flags)) != 0) {
		ssl_error e;

		e.fatal = (ERR_FATAL_ERROR(code) != 0);
		e.error = ERR_GET_REASON(code);
		e.line = line;
		ptr = err_lib_string(code);
		if (ptr) e.lib = ptr;
		ptr = err_fcn_string(code);
		if (ptr) e.fcn = ptr;
		if (file) e.file = file;
		if (data && (flags & ERR_TXT_STRING)) e.data = data;
		ptr = err_reason_string(code);
		if (ptr) e.errstr = ptr;

		m_error.push_back(e);

		flags = 0;
		line = 0;
		file = nullptr;
		data = nullptr;
	}

	err_clear();
}

const char *
ssl_library::library_name(const std::string &id)
{
	const char *libname = nullptr;
	const char *env = getenv(id.c_str());			

	if (env && *env) {
		libname = env;
	} else if (id == "LIBCRYPTO") {
		libname = LIBCRYPTO;
	} else if (id == "LIBSSL") {
		libname = LIBSSL;
	}

	return libname;
}

void
ssl_library::cleanup()
{
	if (m_crypto) {
		reinterpret_cast<p_free_error_strings>(m_crypto->symbol("ERR_free_strings"))();
		delete m_crypto;
		m_crypto = nullptr;
	}

	if (m_ssl) {
		delete m_ssl;
		m_ssl = nullptr;
	}
}

ssl_library::ssl_library()
{
	m_crypto = DBG_NEW snf::dll(library_name("LIBCRYPTO"));
	m_ssl = DBG_NEW snf::dll(library_name("LIBSSL"));

	if (m_ssl) {
		reinterpret_cast<p_library_init>(m_ssl->symbol("SSL_library_init"))();
		reinterpret_cast<p_load_error_strings>(m_ssl->symbol("SSL_load_error_strings"))();
	}
}

/*
 * Get the OpenSSL version.
 *
 * @param [out] ver_str - OpenSSL version string.
 *
 * @return OpenSSL version number.
 */
unsigned long
ssl_library::openssl_version(std::string &ver_str)
{
	ver_str = reinterpret_cast<p_openssl_version_str>(m_crypto->symbol("OpenSSL_version"))(OPENSSL_VERSION);
	return reinterpret_cast<p_openssl_version_num>(m_crypto->symbol("OpenSSL_version_num"))();
}

unsigned long
ssl_library::peek_error()
{
	return reinterpret_cast<p_err_peek>(m_crypto->symbol("ERR_peek_error"))();
}

} // namespace ssl
} // namespace snf
