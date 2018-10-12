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
	const char *ptr;

	while ((code = ssl_library::instance().err_line_data()(&file, &line, &data, &flags)) != 0) {
		ssl_error e;

		e.fatal = (ERR_FATAL_ERROR(code) != 0);
		e.error = ERR_GET_REASON(code);
		e.line = line;
		ptr = ssl_library::instance().err_lib_string()(code);
		if (ptr) e.lib = ptr;
		ptr = ssl_library::instance().err_fcn_string()(code);
		if (ptr) e.fcn = ptr;
		if (file) e.file = file;
		if (data && (flags & ERR_TXT_STRING)) e.data = data;
		ptr = ssl_library::instance().err_reason_string()(code);
		if (ptr) e.errstr = ptr;

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

p_openssl_version_num
ssl_library::openssl_version_num()
{
	if (!m_openssl_version_num)
		m_openssl_version_num = reinterpret_cast<p_openssl_version_num>
			(m_ssl->symbol("OpenSSL_version_num"));
	return m_openssl_version_num;
}

p_openssl_version_str
ssl_library::openssl_version_str()
{
	if (!m_openssl_version_str)
		m_openssl_version_str = reinterpret_cast<p_openssl_version_str>
			(m_ssl->symbol("OpenSSL_version"));
	return m_openssl_version_str;
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

p_bio_s_mem
ssl_library::bio_s_mem()
{
	if (!m_bio_s_mem)
		m_bio_s_mem = reinterpret_cast<p_bio_s_mem>
			(m_ssl->symbol("BIO_s_mem"));
	return m_bio_s_mem;
}

p_bio_new
ssl_library::bio_new()
{
	if (!m_bio_new)
		m_bio_new = reinterpret_cast<p_bio_new>
			(m_ssl->symbol("BIO_new"));
	return m_bio_new;
}

p_bio_read
ssl_library::bio_read()
{
	if (!m_bio_read)
		m_bio_read = reinterpret_cast<p_bio_read>
			(m_ssl->symbol("BIO_read"));
	return m_bio_read;
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

p_evp_pkey_up_ref
ssl_library::evp_pkey_up_ref()
{
	if (!m_evp_pkey_up_ref)
		m_evp_pkey_up_ref = reinterpret_cast<p_evp_pkey_up_ref>
			(m_ssl->symbol("EVP_PKEY_up_ref"));
	return m_evp_pkey_up_ref;
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

p_d2i_x509_fp
ssl_library::d2i_x509_fp()
{
	if (!m_d2i_x509_fp)
		m_d2i_x509_fp = reinterpret_cast<p_d2i_x509_fp>
			(m_ssl->symbol("d2i_X509_fp"));
	return m_d2i_x509_fp;
}

p_d2i_x509
ssl_library::d2i_x509()
{
	if (!m_d2i_x509)
		m_d2i_x509 = reinterpret_cast<p_d2i_x509>
			(m_ssl->symbol("d2i_X509"));
	return m_d2i_x509;
}

p_pem_read_x509
ssl_library::pem_read_x509()
{
	if (!m_pem_read_x509)
		m_pem_read_x509 = reinterpret_cast<p_pem_read_x509>
			(m_ssl->symbol("PEM_read_X509"));
	return m_pem_read_x509;
}

p_pem_read_bio_x509
ssl_library::pem_read_bio_x509()
{
	if (!m_pem_read_bio_x509)
		m_pem_read_bio_x509 = reinterpret_cast<p_pem_read_bio_x509>
			(m_ssl->symbol("PEM_read_bio_X509"));
	return m_pem_read_bio_x509;
}

p_x509_up_ref
ssl_library::x509_up_ref()
{
	if (!m_x509_up_ref)
		m_x509_up_ref = reinterpret_cast<p_x509_up_ref>
			(m_ssl->symbol("X509_up_ref"));
	return m_x509_up_ref;
}

p_x509_free
ssl_library::x509_free()
{
	if (!m_x509_free)
		m_x509_free = reinterpret_cast<p_x509_free>
			(m_ssl->symbol("X509_free"));
	return m_x509_free;
}

p_x509_get_subject
ssl_library::x509_get_subject()
{
	if (!m_x509_get_subject)
		m_x509_get_subject = reinterpret_cast<p_x509_get_subject>
			(m_ssl->symbol("X509_get_subject_name"));
	return m_x509_get_subject;
}

p_x509_get_issuer
ssl_library::x509_get_issuer()
{
	if (!m_x509_get_issuer)
		m_x509_get_issuer = reinterpret_cast<p_x509_get_issuer>
			(m_ssl->symbol("X509_get_issuer_name"));
	return m_x509_get_issuer;
}

p_x509_name_get
ssl_library::x509_name_get()
{
	if (!m_x509_name_get)
		m_x509_name_get = reinterpret_cast<p_x509_name_get>
			(m_ssl->symbol("X509_NAME_print_ex"));
	return m_x509_name_get;
}

p_x509_name_get_text_by_nid
ssl_library::x509_name_get_text_by_nid()
{
	if (!m_x509_name_get_text_by_nid)
		m_x509_name_get_text_by_nid = reinterpret_cast<p_x509_name_get_text_by_nid>
			(m_ssl->symbol("X509_NAME_get_text_by_NID"));
	return m_x509_name_get_text_by_nid;
}

p_x509_get_serial
ssl_library::x509_get_serial()
{
	if (!m_x509_get_serial)
		m_x509_get_serial = reinterpret_cast<p_x509_get_serial>
			(m_ssl->symbol("X509_get_serialNumber"));
	return m_x509_get_serial;
}

p_x509_get_ext_d2i
ssl_library::x509_get_ext_d2i()
{
	if (!m_x509_get_ext_d2i)
		m_x509_get_ext_d2i = reinterpret_cast<p_x509_get_ext_d2i>
			(m_ssl->symbol("X509_get_ext_d2i"));
	return m_x509_get_ext_d2i;
}

p_pem_read_x509_crl
ssl_library::pem_read_x509_crl()
{
	if (!m_pem_read_x509_crl)
		m_pem_read_x509_crl = reinterpret_cast<p_pem_read_x509_crl>
			(m_ssl->symbol("PEM_read_X509_CRL"));
	return m_pem_read_x509_crl;
}

p_pem_read_bio_x509_crl
ssl_library::pem_read_bio_x509_crl()
{
	if (!m_pem_read_bio_x509_crl)
		m_pem_read_bio_x509_crl = reinterpret_cast<p_pem_read_bio_x509_crl>
			(m_ssl->symbol("PEM_read_X509_CRL"));
	return m_pem_read_bio_x509_crl;
}

p_x509_crl_up_ref
ssl_library::x509_crl_up_ref()
{
	if (!m_x509_crl_up_ref)
		m_x509_crl_up_ref = reinterpret_cast<p_x509_crl_up_ref>
			(m_ssl->symbol("X509_CRL_up_ref"));
	return m_x509_crl_up_ref;
}

p_x509_crl_free
ssl_library::x509_crl_free()
{
	if (!m_x509_crl_free)
		m_x509_crl_free = reinterpret_cast<p_x509_crl_free>
			(m_ssl->symbol("X509_CRL_free"));
	return m_x509_crl_free;
}

p_x509_store_new
ssl_library::x509_store_new()
{
	if (!m_x509_store_new)
		m_x509_store_new = reinterpret_cast<p_x509_store_new>
			(m_ssl->symbol("X509_STORE_new"));
	return m_x509_store_new;
}

p_x509_store_up_ref
ssl_library::x509_store_up_ref()
{
	if (!m_x509_store_up_ref)
		m_x509_store_up_ref = reinterpret_cast<p_x509_store_up_ref>
			(m_ssl->symbol("X509_STORE_up_ref"));
	return m_x509_store_up_ref;
}

p_x509_store_free
ssl_library::x509_store_free()
{
	if (!m_x509_store_free)
		m_x509_store_free = reinterpret_cast<p_x509_store_free>
			(m_ssl->symbol("X509_STORE_free"));
	return m_x509_store_free;
}

p_x509_store_load
ssl_library::x509_store_load()
{
	if (!m_x509_store_load)
		m_x509_store_load = reinterpret_cast<p_x509_store_load>
			(m_ssl->symbol("X509_STORE_load_locations"));
	return m_x509_store_load;
}

p_x509_store_add_cert
ssl_library::x509_store_add_cert()
{
	if (!m_x509_store_add_cert)
		m_x509_store_add_cert = reinterpret_cast<p_x509_store_add_cert>
			(m_ssl->symbol("X509_STORE_add_cert"));
	return m_x509_store_add_cert;
}

p_x509_store_add_crl
ssl_library::x509_store_add_crl()
{
	if (!m_x509_store_add_crl)
		m_x509_store_add_crl = reinterpret_cast<p_x509_store_add_crl>
			(m_ssl->symbol("X509_STORE_add_crl"));
	return m_x509_store_add_crl;
}

p_stk_num
ssl_library::stk_num()
{
	if (!m_stk_num)
		m_stk_num = reinterpret_cast<p_stk_num>
			(m_ssl->symbol("sk_num"));
	return m_stk_num;
}

p_stk_val
ssl_library::stk_val()
{
	if (!m_stk_val)
		m_stk_val = reinterpret_cast<p_stk_val>
			(m_ssl->symbol("sk_value"));
	return m_stk_val;
}

p_stk_deep_free
ssl_library::stk_deep_free()
{
	if (!m_stk_deep_free)
		m_stk_deep_free = reinterpret_cast<p_stk_deep_free>
			(m_ssl->symbol("sk_pop_free"));
	return m_stk_deep_free;
}

p_gen_name_free
ssl_library::gen_name_free()
{
	if (!m_gen_name_free)
		m_gen_name_free = reinterpret_cast<p_gen_name_free>
			(m_ssl->symbol("GENERAL_NAME_free"));
	return m_gen_name_free;
}

p_crl_dps_free
ssl_library::crl_dps_free()
{
	if (!m_crl_dps_free)
		m_crl_dps_free = reinterpret_cast<p_crl_dps_free>
			(m_ssl->symbol("CRL_DIST_POINTS_free"));
	return m_crl_dps_free;
}

p_aia_free
ssl_library::aia_free()
{
	if (!m_aia_free)
		m_aia_free = reinterpret_cast<p_aia_free>
			(m_ssl->symbol("AUTHORITY_INFO_ACCESS_free"));
	return m_aia_free;
}

p_obj2nid
ssl_library::obj2nid()
{
	if (!m_obj2nid)
		m_obj2nid = reinterpret_cast<p_obj2nid>
			(m_ssl->symbol("OBJ_obj2nid"));
	return m_obj2nid;
}

p_asn1_string_type
ssl_library::asn1_string_type()
{
	if (!m_asn1_string_type)
		m_asn1_string_type = reinterpret_cast<p_asn1_string_type>
			(m_ssl->symbol("ASN1_STRING_type"));
	return m_asn1_string_type;
}

p_asn1_string_len
ssl_library::asn1_string_len()
{
	if (!m_asn1_string_len)
		m_asn1_string_len = reinterpret_cast<p_asn1_string_len>
			(m_ssl->symbol("ASN1_STRING_length"));
	return m_asn1_string_len;
}

p_asn1_string_val
ssl_library::asn1_string_val()
{
	if (!m_asn1_string_val)
		m_asn1_string_val = reinterpret_cast<p_asn1_string_val>
			(m_ssl->symbol("ASN1_STRING_get0_data"));
	return m_asn1_string_val;
}

p_asn1_integer_to_bn
ssl_library::asn1_integer_to_bn()
{
	if (!m_asn1_integer_to_bn)
		m_asn1_integer_to_bn = reinterpret_cast<p_asn1_integer_to_bn>
			(m_ssl->symbol("ASN1_INTEGER_to_BN"));
	return m_asn1_integer_to_bn;
}

p_bn_to_asn1_integer
ssl_library::bn_to_asn1_integer()
{
	if (!m_bn_to_asn1_integer)
		m_bn_to_asn1_integer = reinterpret_cast<p_bn_to_asn1_integer>
			(m_ssl->symbol("BN_to_ASN1_INTEGER"));
	return m_bn_to_asn1_integer;
}

p_bn2hex
ssl_library::bn2hex()
{
	if (!m_bn2hex)
		m_bn2hex = reinterpret_cast<p_bn2hex>
			(m_ssl->symbol("BN_bn2hex"));
	return m_bn2hex;
}

p_hex2bn
ssl_library::hex2bn()
{
	if (!m_hex2bn)
		m_hex2bn = reinterpret_cast<p_hex2bn>
			(m_ssl->symbol("BN_hex2bn"));
	return m_hex2bn;
}

p_bn_free
ssl_library::bn_free()
{
	if (!m_bn_free)
		m_bn_free = reinterpret_cast<p_bn_free>
			(m_ssl->symbol("BN_free"));
	return m_bn_free;
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

p_tls_method
ssl_library::tls_method()
{
	if (!m_tls_method)
		m_tls_method = reinterpret_cast<p_tls_method>
			(m_ssl->symbol("TLS_method"));
	return m_tls_method;
}

p_ssl_ctx_new
ssl_library::ssl_ctx_new()
{
	if (!m_ssl_ctx_new)
		m_ssl_ctx_new = reinterpret_cast<p_ssl_ctx_new>
			(m_ssl->symbol("SSL_CTX_new"));
	return m_ssl_ctx_new;
}

p_ssl_ctx_up_ref
ssl_library::ssl_ctx_up_ref()
{
	if (!m_ssl_ctx_up_ref)
		m_ssl_ctx_up_ref = reinterpret_cast<p_ssl_ctx_up_ref>
			(m_ssl->symbol("SSL_CTX_up_ref"));
	return m_ssl_ctx_up_ref;
}

p_ssl_ctx_free
ssl_library::ssl_ctx_free()
{
	if (!m_ssl_ctx_free)
		m_ssl_ctx_free = reinterpret_cast<p_ssl_ctx_free>
			(m_ssl->symbol("SSL_CTX_free"));
	return m_ssl_ctx_free;
}

p_ssl_ctx_set_min_ver
ssl_library::ssl_ctx_set_min_ver()
{
	if (!m_ssl_ctx_set_min_ver)
		m_ssl_ctx_set_min_ver = reinterpret_cast<p_ssl_ctx_set_min_ver>
			(m_ssl->symbol("SSL_CTX_set_min_proto_version"));
	return m_ssl_ctx_set_min_ver;
}

p_ssl_ctx_set_max_ver
ssl_library::ssl_ctx_set_max_ver()
{
	if (!m_ssl_ctx_set_max_ver)
		m_ssl_ctx_set_max_ver = reinterpret_cast<p_ssl_ctx_set_max_ver>
			(m_ssl->symbol("SSL_CTX_set_max_proto_version"));
	return m_ssl_ctx_set_max_ver;
}

p_ssl_ctx_ctrl
ssl_library::ssl_ctx_ctrl()
{
	if (!m_ssl_ctx_ctrl)
		m_ssl_ctx_ctrl = reinterpret_cast<p_ssl_ctx_ctrl>
			(m_ssl->symbol("SSL_CTX_ctrl"));
	return m_ssl_ctx_ctrl;
}

p_ssl_ctx_get_options
ssl_library::ssl_ctx_get_options()
{
	if (!m_ssl_ctx_get_options)
		m_ssl_ctx_get_options = reinterpret_cast<p_ssl_ctx_get_options>
			(m_ssl->symbol("SSL_CTX_get_options"));
	return m_ssl_ctx_get_options;
}

p_ssl_ctx_clr_options
ssl_library::ssl_ctx_clr_options()
{
	if (!m_ssl_ctx_clr_options)
		m_ssl_ctx_clr_options = reinterpret_cast<p_ssl_ctx_clr_options>
			(m_ssl->symbol("SSL_CTX_clear_options"));
	return m_ssl_ctx_clr_options;
}

p_ssl_ctx_set_options
ssl_library::ssl_ctx_set_options()
{
	if (!m_ssl_ctx_set_options)
		m_ssl_ctx_set_options = reinterpret_cast<p_ssl_ctx_set_options>
			(m_ssl->symbol("SSL_CTX_set_options"));
	return m_ssl_ctx_set_options;
}

p_ssl_ctx_set_ciphers
ssl_library::ssl_ctx_set_ciphers()
{
	if (!m_ssl_ctx_set_ciphers)
		m_ssl_ctx_set_ciphers = reinterpret_cast<p_ssl_ctx_set_ciphers>
			(m_ssl->symbol("SSL_CTX_set_cipher_list"));
	return m_ssl_ctx_set_ciphers;
}

p_ssl_ctx_use_private_key
ssl_library::ssl_ctx_use_private_key()
{
	if (!m_ssl_ctx_use_private_key)
		m_ssl_ctx_use_private_key = reinterpret_cast<p_ssl_ctx_use_private_key>
			(m_ssl->symbol("SSL_CTX_use_PrivateKey"));
	return m_ssl_ctx_use_private_key;
}

p_ssl_ctx_use_certificate
ssl_library::ssl_ctx_use_certificate()
{
	if (!m_ssl_ctx_use_certificate)
		m_ssl_ctx_use_certificate = reinterpret_cast<p_ssl_ctx_use_certificate>
			(m_ssl->symbol("SSL_CTX_use_certificate"));
	return m_ssl_ctx_use_certificate;
}

p_ssl_ctx_use_truststore
ssl_library::ssl_ctx_use_truststore()
{
	if (!m_ssl_ctx_use_truststore)
		m_ssl_ctx_use_truststore = reinterpret_cast<p_ssl_ctx_use_truststore>
			(m_ssl->symbol("SSL_CTX_set_cert_store"));
	return m_ssl_ctx_use_truststore;
}

p_ssl_ctx_check_private_key
ssl_library::ssl_ctx_check_private_key()
{
	if (!m_ssl_ctx_check_private_key)
		m_ssl_ctx_check_private_key = reinterpret_cast<p_ssl_ctx_check_private_key>
			(m_ssl->symbol("SSL_CTX_check_private_key"));
	return m_ssl_ctx_check_private_key;
}

p_ssl_ctx_set_verify
ssl_library::ssl_ctx_set_verify()
{
	if (!m_ssl_ctx_set_verify)
		m_ssl_ctx_set_verify = reinterpret_cast<p_ssl_ctx_set_verify>
			(m_ssl->symbol("SSL_CTX_set_verify"));
	return m_ssl_ctx_set_verify;
}

p_ssl_ctx_set_verify_depth
ssl_library::ssl_ctx_set_verify_depth()
{
	if (!m_ssl_ctx_set_verify_depth)
		m_ssl_ctx_set_verify_depth = reinterpret_cast<p_ssl_ctx_set_verify_depth>
			(m_ssl->symbol("SSL_CTX_set_verify_depth"));
	return m_ssl_ctx_set_verify_depth;
}

p_ssl_ctx_get_cert_store
ssl_library::ssl_ctx_get_cert_store()
{
	if (!m_ssl_ctx_get_cert_store)
		m_ssl_ctx_get_cert_store = reinterpret_cast<p_ssl_ctx_get_cert_store>
			(m_ssl->symbol("SSL_CTX_get_cert_store"));
	return m_ssl_ctx_get_cert_store;
}

} // namespace ssl
} // namespace net
} // namespace snf