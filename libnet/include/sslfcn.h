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
#include <openssl/stack.h>
#include <openssl/asn1.h>

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
using p_x509_get_ext_d2i = void * (*)(const X509 *, int, int *, int *);

using p_stk_num = int (*)(const _STACK *);
using p_stk_val = void * (*)(const _STACK *, int);
using p_stk_deep_free = void (*)(_STACK *, void (*)(void *));
using p_gen_name_free = void (*)(void *);

using p_asn1_string_type = int (*)(const ASN1_STRING *);
using p_asn1_string_len = int (*)(const ASN1_STRING *);
using p_asn1_string_val = const unsigned char * (*)(const ASN1_STRING *);

using p_err_line_data = unsigned long (*)(const char **, int *, const char **, int *);
using p_err_lib_string = const char * (*)(unsigned long);
using p_err_fcn_string = const char * (*)(unsigned long);
using p_err_reason_string = const char * (*)(unsigned long);
using p_err_clear = void (*)(void);

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
	p_x509_get_ext_d2i          m_x509_get_ext_d2i = nullptr;

	p_stk_num                   m_stk_num = nullptr;
	p_stk_val                   m_stk_val = nullptr;
	p_stk_deep_free             m_stk_deep_free = nullptr;
	p_gen_name_free             m_gen_name_free = nullptr;

	p_asn1_string_type          m_asn1_string_type = nullptr;
	p_asn1_string_len           m_asn1_string_len = nullptr;
	p_asn1_string_val           m_asn1_string_val = nullptr;

	p_err_line_data             m_err_line_data = nullptr;
	p_err_lib_string            m_err_lib_string = nullptr;
	p_err_fcn_string            m_err_fcn_string = nullptr;
	p_err_reason_string         m_err_reason_string = nullptr;
	p_err_clear                 m_err_clear = nullptr;

	ssl_library();

	void cleanup() { if (m_ssl) { delete m_ssl; m_ssl = nullptr; } }

public:
	static ssl_library &instance()
	{
		static ssl_library ssllib;
		return ssllib;
	}

	~ssl_library() { cleanup(); }

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
	p_x509_get_ext_d2i x509_get_ext_d2i();

	p_stk_num stk_num();
	p_stk_val stk_val();
	p_stk_deep_free stk_deep_free();
	p_gen_name_free gen_name_free();

	p_asn1_string_type asn1_string_type();
	p_asn1_string_len asn1_string_len();
	p_asn1_string_val asn1_string_val();

	p_err_line_data err_line_data();
	p_err_lib_string err_lib_string();
	p_err_fcn_string err_fcn_string();
	p_err_reason_string err_reason_string();
	p_err_clear err_clear();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SSLFCN_H_
