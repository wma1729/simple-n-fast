#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

void
ssl_exception::init()
{
	unsigned long code;
	int flags = 0, line = 0;
	const char *file = nullptr, *data = nullptr;

	while ((code = ssl_library::instance().err_line_data()(&file, &line, &data, &flags)) != 0) {
		ssl_error e;

		e.fatal = (ERR_FATAL_ERROR(code) != 0);
		e.error = ERR_GET_REASON(code);
		e.line = line;
		e.lib = ssl_library::instance().err_lib_string()(code);
		e.fcn = ssl_library::instance().err_fcn_string()(code);
		if (file) e.file = file;
		if (data && (flags & ERR_TXT_STRING)) e.data = data;
		e.errstr = ssl_library::instance().err_reason_string()(code);

		m_error.push_back(e);

		flags = 0;
		line = 0;
		file = nullptr;
		data = nullptr;
	}

	ssl_library::instance().err_clear()();
}

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

p_bio_new_mem_buf
ssl_library::bio_new_mem_buf()
{
	if (!m_bio_new_mem_buf)
		m_bio_new_mem_buf = reinterpret_cast<p_bio_new_mem_buf>
			(m_ssl->symbol("BIO_new_mem_buf"));
	return m_bio_new_mem_buf;
}

p_bio_free
ssl_library::bio_free()
{
	if (!m_bio_free)
		m_bio_free = reinterpret_cast<p_bio_free>
			(m_ssl->symbol("BIO_free"));
	return m_bio_free;
}

p_d2i_private_key_fp
ssl_library::d2i_private_key_fp()
{
	if (!m_d2i_private_key_fp)
		m_d2i_private_key_fp = reinterpret_cast<p_d2i_private_key_fp>
			(m_ssl->symbol("d2i_PrivateKey_fp"));
	return m_d2i_private_key_fp;
}

p_d2i_auto_private_key
ssl_library::d2i_auto_private_key()
{
	if (!m_d2i_auto_private_key)
		m_d2i_auto_private_key = reinterpret_cast<p_d2i_auto_private_key>
			(m_ssl->symbol("d2i_AutoPrivateKey"));
	return m_d2i_auto_private_key;
}

p_pem_read_private_key
ssl_library::pem_read_private_key()
{
	if (!m_pem_read_private_key)
		m_pem_read_private_key = reinterpret_cast<p_pem_read_private_key>
			(m_ssl->symbol("PEM_read_PrivateKey"));
	return m_pem_read_private_key;
}

p_pem_read_bio_private_key
ssl_library::pem_read_bio_private_key()
{
	if (!m_pem_read_bio_private_key)
		m_pem_read_bio_private_key = reinterpret_cast<p_pem_read_bio_private_key>
			(m_ssl->symbol("PEM_read_bio_PrivateKey"));
	return m_pem_read_bio_private_key;
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

p_get1_rsa
ssl_library::get1_rsa()
{
	if (!m_get1_rsa)
		m_get1_rsa = reinterpret_cast<p_get1_rsa>
			(m_ssl->symbol("EVP_PKEY_get1_RSA"));
	return m_get1_rsa;
}

p_rsa_free
ssl_library::rsa_free()
{
	if (!m_rsa_free)
		m_rsa_free = reinterpret_cast<p_rsa_free>
			(m_ssl->symbol("RSA_free"));
	return m_rsa_free;
}

p_rsa_key_check
ssl_library::rsa_key_check()
{
	if (!m_rsa_key_check)
		m_rsa_key_check = reinterpret_cast<p_rsa_key_check>
			(m_ssl->symbol("RSA_check_key"));
	return m_rsa_key_check;
}

p_get1_dh
ssl_library::get1_dh()
{
	if (!m_get1_dh)
		m_get1_dh = reinterpret_cast<p_get1_dh>
			(m_ssl->symbol("EVP_PKEY_get1_DH"));
	return m_get1_dh;
}

p_dh_free
ssl_library::dh_free()
{
	if (!m_dh_free)
		m_dh_free = reinterpret_cast<p_dh_free>
			(m_ssl->symbol("DH_free"));
	return m_dh_free;
}

p_dh_key_check
ssl_library::dh_key_check()
{
	if (!m_dh_key_check)
		m_dh_key_check = reinterpret_cast<p_dh_key_check>
			(m_ssl->symbol("DH_check"));
	return m_dh_key_check;
}

p_get1_ec_key
ssl_library::get1_ec_key()
{
	if (!m_get1_ec_key)
		m_get1_ec_key = reinterpret_cast<p_get1_ec_key>
			(m_ssl->symbol("EVP_PKEY_get1_EC_KEY"));
	return m_get1_ec_key;
}

p_ec_key_free
ssl_library::ec_key_free()
{
	if (!m_ec_key_free)
		m_ec_key_free = reinterpret_cast<p_ec_key_free>
			(m_ssl->symbol("EC_KEY_free"));
	return m_ec_key_free;
}

p_ec_key_check
ssl_library::ec_key_check()
{
	if (!m_ec_key_check)
		m_ec_key_check = reinterpret_cast<p_ec_key_check>
			(m_ssl->symbol("EC_KEY_check_key"));
	return m_ec_key_check;
}

p_err_line_data
ssl_library::err_line_data()
{
	if (!m_err_line_data)
		m_err_line_data = reinterpret_cast<p_err_line_data>
			(m_ssl->symbol("ERR_get_error_line_data"));
	return m_err_line_data;
}

p_err_lib_string
ssl_library::err_lib_string()
{
	if (!m_err_lib_string)
		m_err_lib_string = reinterpret_cast<p_err_lib_string>
			(m_ssl->symbol("ERR_lib_error_string"));
	return m_err_lib_string;
}

p_err_fcn_string
ssl_library::err_fcn_string()
{
	if (!m_err_fcn_string)
		m_err_fcn_string = reinterpret_cast<p_err_fcn_string>
			(m_ssl->symbol("ERR_func_error_string"));
	return m_err_fcn_string;
}

p_err_reason_string
ssl_library::err_reason_string()
{
	if (!m_err_reason_string)
		m_err_reason_string = reinterpret_cast<p_err_reason_string>
			(m_ssl->symbol("ERR_reason_error_string"));
	return m_err_reason_string;
}

p_err_clear
ssl_library::err_clear()
{
	if (!m_err_clear)
		m_err_clear = reinterpret_cast<p_err_clear>
			(m_ssl->symbol("ERR_clear_error"));
	return m_err_clear;
}

} // namespace ssl
} // namespace net
} // namespace snf
