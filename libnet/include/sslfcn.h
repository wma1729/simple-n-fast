#ifndef _SNF_SSLFCN_H_
#define _SNF_SSLFCN_H_

#include "dll.h"
#include <vector>
#include <stdexcept>
#include <ostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/stack.h>
#include <openssl/asn1.h>
#include <openssl/bn.h>

using p_openssl_version_num = unsigned long (*)(void);
using p_openssl_version_str = const char * (*)(int);

using p_library_init = int (*)(void);
using p_load_error_strings = void (*)(void);
using p_free_error_strings = void (*)(void);

using p_bio_s_mem = const BIO_METHOD * (*)(void);
using p_bio_new = BIO * (*)(const BIO_METHOD *);
using p_bio_read = int (*)(BIO *, void *, int);
using p_bio_new_mem_buf = BIO * (*)(const void *, int);
using p_bio_free = int (*)(BIO *);

using p_d2i_private_key_fp = EVP_PKEY * (*)(FILE *, EVP_PKEY **);
using p_d2i_auto_private_key = EVP_PKEY * (*)(EVP_PKEY **, const unsigned char **, long);
using p_pem_read_private_key = EVP_PKEY * (*)(FILE *, EVP_PKEY **, pem_password_cb *, void *);
using p_pem_read_bio_private_key = EVP_PKEY * (*)(BIO *, EVP_PKEY **, pem_password_cb *, void *);
using p_evp_pkey_up_ref = int (*)(EVP_PKEY *);
using p_evp_pkey_base_id = int (*)(EVP_PKEY *);
using p_evp_pkey_free = void (*)(EVP_PKEY *);
using p_get1_rsa = RSA * (*)(EVP_PKEY *);
using p_rsa_free = void (*)(RSA *);
using p_rsa_key_check = int (*)(RSA *);
using p_get1_dh = DH * (*)(EVP_PKEY *);
using p_dh_free = void (*)(DH *);
using p_dh_key_check = int (*)(DH *, int *);
using p_get1_ec_key = EC_KEY * (*)(EVP_PKEY *);
using p_ec_key_free = void (*)(EC_KEY *);
using p_ec_key_check = int (*)(const EC_KEY *);

using p_d2i_x509_fp = X509 * (*)(FILE *, X509 **);
using p_d2i_x509 = X509 * (*)(X509 **, const unsigned char **, long);
using p_pem_read_x509 = X509 * (*)(FILE *, X509 **, pem_password_cb *, void *);
using p_pem_read_bio_x509 = X509 * (*)(BIO *, X509 **, pem_password_cb *, void *);
using p_x509_up_ref = int (*)(X509 *);
using p_x509_free = void (*)(X509 *);
using p_x509_get_subject = X509_NAME * (*)(const X509 *);
using p_x509_get_issuer = X509_NAME * (*)(const X509 *);
using p_x509_name_get = int (*)(BIO *, const X509_NAME *, int, unsigned long);
using p_x509_name_get_text_by_nid = int (*)(X509_NAME *, int, char *, int);
using p_x509_get_serial = ASN1_INTEGER * (*)(X509 *);
using p_x509_get_ext_d2i = void * (*)(const X509 *, int, int *, int *);

using p_pem_read_x509_crl = X509_CRL * (*)(FILE *, X509_CRL **, pem_password_cb *, void *);
using p_pem_read_bio_x509_crl = X509_CRL * (*)(BIO *, X509_CRL **, pem_password_cb *, void *);
using p_x509_crl_up_ref = int (*)(X509_CRL *);
using p_x509_crl_free = void (*)(X509_CRL *);

using p_x509_store_new = X509_STORE * (*)(void);
using p_x509_store_up_ref = int (*)(X509_STORE *);
using p_x509_store_free = void (*)(X509_STORE *);
using p_x509_store_load = int (*)(X509_STORE *, const char *, const char *);
using p_x509_store_add_cert = int (*)(X509_STORE *, X509 *);
using p_x509_store_add_crl = int (*)(X509_STORE *, X509_CRL *);
using p_x509_store_set_flags = int (*)(X509_STORE *, unsigned long);

using p_stk_num = int (*)(const _STACK *);
using p_stk_val = void * (*)(const _STACK *, int);
using p_stk_deep_free = void (*)(_STACK *, void (*)(void *));
using p_gen_name_free = void (*)(void *);
using p_crl_dps_free = void (*)(void *);
using p_aia_free = void (*)(void *);

using p_obj2nid = int (*)(const ASN1_OBJECT *);
using p_asn1_string_type = int (*)(const ASN1_STRING *);
using p_asn1_string_len = int (*)(const ASN1_STRING *);
using p_asn1_string_val = const unsigned char * (*)(const ASN1_STRING *);
using p_asn1_integer_to_bn = BIGNUM * (*)(const ASN1_INTEGER *, BIGNUM *);
using p_bn_to_asn1_integer = ASN1_INTEGER * (*)(const BIGNUM *, ASN1_INTEGER *);
using p_bn2hex = char * (*)(const BIGNUM *);
using p_hex2bn = int (*)(BIGNUM **, const char *);
using p_bn_free = void (*)(BIGNUM *);

using p_err_line_data = unsigned long (*)(const char **, int *, const char **, int *);
using p_err_lib_string = const char * (*)(unsigned long);
using p_err_fcn_string = const char * (*)(unsigned long);
using p_err_reason_string = const char * (*)(unsigned long);
using p_err_clear = void (*)(void);

using p_tls_method = const SSL_METHOD * (*)(void);
using p_ssl_ctx_new = SSL_CTX * (*)(const SSL_METHOD *);
using p_ssl_ctx_up_ref = int (*)(SSL_CTX *);
using p_ssl_ctx_free = void (*)(SSL_CTX *);
using p_ssl_ctx_set_min_ver = int (*)(SSL_CTX *, uint16_t);
using p_ssl_ctx_set_max_ver = int (*)(SSL_CTX *, uint16_t);
using p_ssl_ctx_ctrl = long (*)(SSL_CTX *, int, long, void *);
using p_ssl_ctx_cb_ctrl = long (*)(SSL_CTX *, int, void (*)(void));
using p_ssl_ctx_get_options = long (*)(const SSL_CTX *);
using p_ssl_ctx_clr_options = long (*)(SSL_CTX *, unsigned long op);
using p_ssl_ctx_set_options = long (*)(SSL_CTX *, unsigned long op);
using p_ssl_ctx_set_ciphers = int (*)(SSL_CTX *, const char *);
using p_ssl_ctx_use_private_key = int (*)(SSL_CTX *, EVP_PKEY *);
using p_ssl_ctx_use_certificate = int (*)(SSL_CTX *, X509 *);
using p_ssl_ctx_use_truststore = void (*)(SSL_CTX *, X509_STORE *);
using p_ssl_ctx_check_private_key = int (*)(const SSL_CTX *);
using p_ssl_ctx_verify_cb = int (*)(X509_STORE_CTX *);
using p_ssl_ctx_set_verify = void (*)(SSL_CTX *, int, p_ssl_ctx_verify_cb);
using p_ssl_ctx_set_verify_depth = void (*)(SSL_CTX *, int);
using p_ssl_ctx_get_cert_store = X509_STORE * (*)(const SSL_CTX *);
using p_ssl_ctx_get0_cert = X509 * (*)(const SSL_CTX *);

using p_ssl_new = SSL * (*)(SSL_CTX *);
using p_ssl_free = void (*)(SSL *);
using p_ssl_set_ssl_ctx = SSL_CTX * (*)(SSL *, SSL_CTX *);
using p_ssl_ctrl = long (*)(SSL *, int, long, void *);
using p_ssl_get_servername = const char * (*)(const SSL *, int);
using p_ssl_set_connect_state = void (*)(SSL *);
using p_ssl_set_accept_state = void (*)(SSL *);

using p_ssl_get0_param = X509_VERIFY_PARAM * (*)(SSL *);
using p_x509_verify_param_set_hostflags = void (*)(X509_VERIFY_PARAM *, unsigned int);
using p_x509_verify_param_set1_host = int (*)(X509_VERIFY_PARAM *, const char *, size_t);
using p_x509_verify_param_add1_host = int (*)(X509_VERIFY_PARAM *, const char *, size_t);
using p_x509_verify_param_set1_ip_asc = int (*)(X509_VERIFY_PARAM *, const char *);

namespace snf {
namespace net {
namespace ssl {

enum class ssl_data_fmt
{
	pem = 1,
	der = 2
};

struct ssl_error
{
	bool fatal;
	int error;
	int line;
	std::string lib;
	std::string fcn;
	std::string file;
	std::string data;
	std::string errstr;
};

inline std::ostream &
operator<< (std::ostream &os, const ssl_error &e)
{
	if (e.fatal)
		os << "Fatal error - ";
	else
		os << "Non-fatal  error - ";
	if (!e.lib.empty())     os << e.lib  << ":";
	if (!e.file.empty())    os << e.file << ":";
	if (!e.fcn.empty())     os << e.fcn  << ":";
	if (e.line != 0)        os << e.line << " ";
	if (!e.data.empty())    os << "[" << e.data << "] ";
	os << e.errstr << "(" << e.error << ")";
	return os;
}

class ssl_exception : public std::runtime_error
{
private:
	std::vector<ssl_error> m_error;

	void init();

public:
	ssl_exception(const std::string &msg) : std::runtime_error(msg) { init(); }
	ssl_exception(const char *msg) : std::runtime_error(msg) { init(); }

	std::vector<ssl_error>::const_iterator begin() const { return m_error.begin(); }
	std::vector<ssl_error>::const_iterator end() const { return m_error.end(); }
};

class ssl_library
{
private:
	snf::dll                    *m_ssl = nullptr;

	p_openssl_version_num       m_openssl_version_num = nullptr;
	p_openssl_version_str       m_openssl_version_str = nullptr;

	p_library_init              m_library_init = nullptr;
	p_load_error_strings        m_load_error_strings = nullptr;
	p_free_error_strings        m_free_error_strings = nullptr;

	p_bio_s_mem                 m_bio_s_mem = nullptr;
	p_bio_new                   m_bio_new = nullptr;
	p_bio_read                  m_bio_read = nullptr;
	p_bio_new_mem_buf           m_bio_new_mem_buf = nullptr;
	p_bio_free                  m_bio_free = nullptr;

	p_d2i_private_key_fp        m_d2i_private_key_fp = nullptr;
	p_d2i_auto_private_key      m_d2i_auto_private_key = nullptr;
	p_pem_read_private_key      m_pem_read_private_key = nullptr;
	p_pem_read_bio_private_key  m_pem_read_bio_private_key = nullptr;
	p_evp_pkey_up_ref           m_evp_pkey_up_ref = nullptr;
	p_evp_pkey_base_id          m_evp_pkey_base_id = nullptr;
	p_evp_pkey_free             m_evp_pkey_free = nullptr;
	p_get1_rsa                  m_get1_rsa = nullptr;
	p_rsa_free                  m_rsa_free = nullptr;
	p_rsa_key_check             m_rsa_key_check = nullptr;
	p_get1_dh                   m_get1_dh = nullptr;
	p_dh_free                   m_dh_free = nullptr;
	p_dh_key_check              m_dh_key_check = nullptr;
	p_get1_ec_key               m_get1_ec_key = nullptr;
	p_ec_key_free               m_ec_key_free = nullptr;
	p_ec_key_check              m_ec_key_check = nullptr;

	p_d2i_x509_fp               m_d2i_x509_fp = nullptr;
	p_d2i_x509                  m_d2i_x509 = nullptr;
	p_pem_read_x509             m_pem_read_x509 = nullptr;
	p_pem_read_bio_x509         m_pem_read_bio_x509 = nullptr;
	p_x509_up_ref               m_x509_up_ref = nullptr;
	p_x509_free                 m_x509_free = nullptr;
	p_x509_get_subject          m_x509_get_subject = nullptr;
	p_x509_get_issuer           m_x509_get_issuer = nullptr;
	p_x509_name_get             m_x509_name_get = nullptr;
	p_x509_name_get_text_by_nid m_x509_name_get_text_by_nid = nullptr;
	p_x509_get_serial           m_x509_get_serial = nullptr;
	p_x509_get_ext_d2i          m_x509_get_ext_d2i = nullptr;

	p_pem_read_x509_crl         m_pem_read_x509_crl = nullptr;
	p_pem_read_bio_x509_crl     m_pem_read_bio_x509_crl = nullptr;
	p_x509_crl_up_ref           m_x509_crl_up_ref = nullptr;
	p_x509_crl_free             m_x509_crl_free = nullptr;

	p_x509_store_new            m_x509_store_new = nullptr;
	p_x509_store_up_ref         m_x509_store_up_ref = nullptr;
	p_x509_store_free           m_x509_store_free = nullptr;
	p_x509_store_load           m_x509_store_load = nullptr;
	p_x509_store_add_cert       m_x509_store_add_cert = nullptr;
	p_x509_store_add_crl        m_x509_store_add_crl = nullptr;
	p_x509_store_set_flags      m_x509_store_set_flags = nullptr;

	p_stk_num                   m_stk_num = nullptr;
	p_stk_val                   m_stk_val = nullptr;
	p_stk_deep_free             m_stk_deep_free = nullptr;
	p_gen_name_free             m_gen_name_free = nullptr;
	p_crl_dps_free              m_crl_dps_free = nullptr;
	p_aia_free                  m_aia_free = nullptr;

	p_obj2nid                   m_obj2nid = nullptr;
	p_asn1_string_type          m_asn1_string_type = nullptr;
	p_asn1_string_len           m_asn1_string_len = nullptr;
	p_asn1_string_val           m_asn1_string_val = nullptr;
	p_asn1_integer_to_bn        m_asn1_integer_to_bn = nullptr;
	p_bn_to_asn1_integer        m_bn_to_asn1_integer = nullptr;
	p_bn2hex                    m_bn2hex = nullptr;
	p_hex2bn                    m_hex2bn = nullptr;
	p_bn_free                   m_bn_free = nullptr;

	p_err_line_data             m_err_line_data = nullptr;
	p_err_lib_string            m_err_lib_string = nullptr;
	p_err_fcn_string            m_err_fcn_string = nullptr;
	p_err_reason_string         m_err_reason_string = nullptr;
	p_err_clear                 m_err_clear = nullptr;

	p_tls_method                m_tls_method = nullptr;
	p_ssl_ctx_new               m_ssl_ctx_new = nullptr;
	p_ssl_ctx_up_ref            m_ssl_ctx_up_ref = nullptr;
	p_ssl_ctx_free              m_ssl_ctx_free = nullptr;
	p_ssl_ctx_set_min_ver       m_ssl_ctx_set_min_ver = nullptr;
	p_ssl_ctx_set_max_ver       m_ssl_ctx_set_max_ver = nullptr;
	p_ssl_ctx_ctrl              m_ssl_ctx_ctrl = nullptr;
	p_ssl_ctx_cb_ctrl           m_ssl_ctx_cb_ctrl = nullptr;
	p_ssl_ctx_get_options       m_ssl_ctx_get_options = nullptr;
	p_ssl_ctx_clr_options       m_ssl_ctx_clr_options = nullptr;
	p_ssl_ctx_set_options       m_ssl_ctx_set_options = nullptr;
	p_ssl_ctx_set_ciphers       m_ssl_ctx_set_ciphers = nullptr;
	p_ssl_ctx_use_private_key   m_ssl_ctx_use_private_key = nullptr;
	p_ssl_ctx_use_certificate   m_ssl_ctx_use_certificate = nullptr;
	p_ssl_ctx_use_truststore    m_ssl_ctx_use_truststore = nullptr;
	p_ssl_ctx_check_private_key m_ssl_ctx_check_private_key = nullptr;
	p_ssl_ctx_set_verify        m_ssl_ctx_set_verify = nullptr;
	p_ssl_ctx_set_verify_depth  m_ssl_ctx_set_verify_depth = nullptr;
	p_ssl_ctx_get_cert_store    m_ssl_ctx_get_cert_store = nullptr;
	p_ssl_ctx_get0_cert         m_ssl_ctx_get0_cert = nullptr;

	p_ssl_new                   m_ssl_new = nullptr;
	p_ssl_free                  m_ssl_free = nullptr;
	p_ssl_set_ssl_ctx           m_ssl_set_ssl_ctx = nullptr;
	p_ssl_ctrl                  m_ssl_ctrl = nullptr;
	p_ssl_get_servername        m_ssl_get_servername = nullptr;
	p_ssl_set_connect_state     m_ssl_set_connect_state = nullptr;
	p_ssl_set_accept_state      m_ssl_set_accept_state = nullptr;

	p_ssl_get0_param                    m_ssl_get0_param = nullptr;
	p_x509_verify_param_set_hostflags   m_x509_verify_param_set_hostflags = nullptr;
	p_x509_verify_param_set1_host       m_x509_verify_param_set1_host = nullptr;
	p_x509_verify_param_add1_host       m_x509_verify_param_add1_host = nullptr;
	p_x509_verify_param_set1_ip_asc     m_x509_verify_param_set1_ip_asc = nullptr;

	ssl_library();

	void cleanup() { if (m_ssl) { delete m_ssl; m_ssl = nullptr; } }

public:
	static ssl_library &instance()
	{
		static ssl_library ssllib;
		return ssllib;
	}

	~ssl_library() { cleanup(); }

	p_openssl_version_num openssl_version_num();
	p_openssl_version_str openssl_version_str();

	p_library_init library_init();
	p_load_error_strings load_error_strings();
	p_free_error_strings free_error_strings();

	p_bio_s_mem bio_s_mem();
	p_bio_new bio_new();
	p_bio_read bio_read();
	p_bio_new_mem_buf bio_new_mem_buf();
	p_bio_free bio_free();

	p_d2i_private_key_fp d2i_private_key_fp();
	p_d2i_auto_private_key d2i_auto_private_key();
	p_pem_read_private_key pem_read_private_key();
	p_pem_read_bio_private_key pem_read_bio_private_key();
	p_evp_pkey_up_ref evp_pkey_up_ref();
	p_evp_pkey_base_id evp_pkey_base_id();
	p_evp_pkey_free evp_pkey_free();
	p_get1_rsa get1_rsa();
	p_rsa_free rsa_free();
	p_rsa_key_check rsa_key_check();
	p_get1_dh get1_dh();
	p_dh_free dh_free();
	p_dh_key_check dh_key_check();
	p_get1_ec_key get1_ec_key();
	p_ec_key_free ec_key_free();
	p_ec_key_check ec_key_check();

	p_d2i_x509_fp d2i_x509_fp();
	p_d2i_x509 d2i_x509();
	p_pem_read_x509 pem_read_x509();
	p_pem_read_bio_x509 pem_read_bio_x509();
	p_x509_up_ref x509_up_ref();
	p_x509_free x509_free();
	p_x509_get_subject x509_get_subject();
	p_x509_get_issuer x509_get_issuer();
	p_x509_name_get x509_name_get();
	p_x509_name_get_text_by_nid x509_name_get_text_by_nid();
	p_x509_get_serial x509_get_serial();
	p_x509_get_ext_d2i x509_get_ext_d2i();

	p_pem_read_x509_crl pem_read_x509_crl();
	p_pem_read_bio_x509_crl pem_read_bio_x509_crl();
	p_x509_crl_up_ref x509_crl_up_ref();
	p_x509_crl_free x509_crl_free();

	p_x509_store_new x509_store_new();
	p_x509_store_up_ref x509_store_up_ref();
	p_x509_store_free x509_store_free();
	p_x509_store_load x509_store_load();
	p_x509_store_add_cert x509_store_add_cert();
	p_x509_store_add_crl x509_store_add_crl();
	p_x509_store_set_flags x509_store_set_flags();

	p_stk_num stk_num();
	p_stk_val stk_val();
	p_stk_deep_free stk_deep_free();
	p_gen_name_free gen_name_free();
	p_crl_dps_free crl_dps_free();
	p_aia_free aia_free();

	p_obj2nid obj2nid();
	p_asn1_string_type asn1_string_type();
	p_asn1_string_len asn1_string_len();
	p_asn1_string_val asn1_string_val();
	p_asn1_integer_to_bn asn1_integer_to_bn();
	p_bn_to_asn1_integer bn_to_asn1_integer();
	p_bn2hex bn2hex();
	p_hex2bn hex2bn();
	p_bn_free bn_free();

	p_err_line_data err_line_data();
	p_err_lib_string err_lib_string();
	p_err_fcn_string err_fcn_string();
	p_err_reason_string err_reason_string();
	p_err_clear err_clear();

	p_tls_method tls_method();
	p_ssl_ctx_new ssl_ctx_new();
	p_ssl_ctx_up_ref ssl_ctx_up_ref();
	p_ssl_ctx_free ssl_ctx_free();
	p_ssl_ctx_set_min_ver ssl_ctx_set_min_ver();
	p_ssl_ctx_set_max_ver ssl_ctx_set_max_ver();
	p_ssl_ctx_ctrl ssl_ctx_ctrl();
	p_ssl_ctx_cb_ctrl ssl_ctx_cb_ctrl();
	p_ssl_ctx_get_options ssl_ctx_get_options();
	p_ssl_ctx_clr_options ssl_ctx_clr_options();
	p_ssl_ctx_set_options ssl_ctx_set_options();
	p_ssl_ctx_set_ciphers ssl_ctx_set_ciphers();
	p_ssl_ctx_use_private_key ssl_ctx_use_private_key();
	p_ssl_ctx_use_certificate ssl_ctx_use_certificate();
	p_ssl_ctx_use_truststore ssl_ctx_use_truststore();
	p_ssl_ctx_check_private_key ssl_ctx_check_private_key();
	p_ssl_ctx_set_verify ssl_ctx_set_verify();
	p_ssl_ctx_set_verify_depth ssl_ctx_set_verify_depth();
	p_ssl_ctx_get_cert_store ssl_ctx_get_cert_store();
	p_ssl_ctx_get0_cert ssl_ctx_get0_cert();

	p_ssl_new ssl_new();
	p_ssl_free ssl_free();
	p_ssl_set_ssl_ctx ssl_set_ssl_ctx();
	p_ssl_ctrl ssl_ctrl();
	p_ssl_get_servername ssl_get_servername();
	p_ssl_set_connect_state ssl_set_connect_state();
	p_ssl_set_accept_state ssl_set_accept_state();

	p_ssl_get0_param ssl_get0_param();
	p_x509_verify_param_set_hostflags x509_verify_param_set_hostflags();
	p_x509_verify_param_set1_host x509_verify_param_set1_host();
	p_x509_verify_param_add1_host x509_verify_param_add1_host();
	p_x509_verify_param_set1_ip_asc x509_verify_param_set1_ip_asc();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SSLFCN_H_
