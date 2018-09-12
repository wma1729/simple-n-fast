#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

ssl_library::ssl_library()
{
	const char *libname = "libssl.so";
	const char *env = getenv("LIBSSL");
	if (env && *env)
		libname = env;
	m_ssl = new snf::dll(libname);
}

ssl_library::~ssl_library()
{
	if (m_ssl)
		delete m_ssl;
}

p_library_init
ssl_library::library_init()
{
	if (!m_library_init)
		m_library_init = reinterpret_cast<p_library_init>
			(m_ssl->symbol("SSL_library_init"));
	return m_library_init;
}

p_load_error_strings
ssl_library::load_error_strings()
{
	if (!m_load_error_strings)
		m_load_error_strings = reinterpret_cast<p_load_error_strings>
			(m_ssl->symbol("SSL_load_error_strings"));
	return m_load_error_strings;
}

p_free_error_strings
ssl_library::free_error_strings()
{
	if (!m_free_error_strings)
		m_free_error_strings = reinterpret_cast<p_free_error_strings>
			(m_ssl->symbol("ERR_free_strings"));
	return m_free_error_strings;
}

p_d2i_private_key_fp
ssl_library::d2i_private_key_fp()
{
	if (!m_d2i_private_key_fp)
		m_d2i_private_key_fp = reinterpret_cast<p_d2i_private_key_fp>
			(m_ssl->symbol("d2i_PrivateKey_fp"));
	return m_d2i_private_key_fp;
}

p_pem_read_privatekey
ssl_library::pem_read_privatekey()
{
	if (!m_pem_read_privatekey)
		m_pem_read_privatekey = reinterpret_cast<p_pem_read_privatekey>
			(m_ssl->symbol("PEM_read_PrivateKey"));
	return m_pem_read_privatekey;
}

p_evp_pkey_base_id
ssl_library::evp_pkey_base_id()
{
	if (!m_evp_pkey_base_id)
		m_evp_pkey_base_id = reinterpret_cast<p_evp_pkey_base_id>
			(m_ssl->symbol("EVP_PKEY_base_id"));
	return m_evp_pkey_base_id;
}

p_evp_pkey_free
ssl_library::evp_pkey_free()
{
	if (!m_evp_pkey_free)
		m_evp_pkey_free = reinterpret_cast<p_evp_pkey_free>
			(m_ssl->symbol("EVP_PKEY_free"));
	return m_evp_pkey_free;
}

} // namespace ssl
} // namespace net
} // namespace snf
