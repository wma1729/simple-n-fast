#ifndef _SNF_SSLFCN_H_
#define _SNF_SSLFCN_H_

#include "dll.h"
#include <vector>
#include <map>
#include <exception>
#include <ostream>
#include <sstream>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/x509_vfy.h>
#include <openssl/stack.h>
#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/hmac.h>

#if defined(_WIN32)

constexpr const char *LIBSSL = "ssl.dll";
constexpr const char *LIBCRYPTO = "crypto.dll";

#else // !_WIN32

constexpr const char *LIBSSL = "libssl.so";
constexpr const char *LIBCRYPTO = "libcrypto.so";

#endif // _WIN32

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
using p_err_peek = unsigned long (*)(void);

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
using p_ssl_ctx_set_sid_ctx = int (*)(SSL_CTX *, const unsigned char *, unsigned int);
using p_ssl_ctx_tlsext_ticket_key_cb = int (*)(SSL *, unsigned char *, unsigned char *, EVP_CIPHER_CTX *, HMAC_CTX *, int);
using p_ssl_ctx_get_timeout = long (*)(const SSL_CTX *);
using p_ssl_ctx_set_timeout = long (*)(SSL_CTX *, long);

using p_ssl_new = SSL * (*)(SSL_CTX *);
using p_ssl_dup = SSL * (*)(SSL *);
using p_ssl_free = void (*)(SSL *);
using p_ssl_set_ssl_ctx = SSL_CTX * (*)(SSL *, SSL_CTX *);
using p_ssl_ctrl = long (*)(SSL *, int, long, void *);
using p_ssl_get_servername = const char * (*)(const SSL *, int);
using p_ssl_set_connect_state = void (*)(SSL *);
using p_ssl_set_accept_state = void (*)(SSL *);
using p_ssl_set_fd = int (*)(SSL *, int);
using p_ssl_get_fd = int (*)(const SSL *);
using p_ssl_connect = int (*)(SSL *);
using p_ssl_accept = int (*)(SSL *);
using p_ssl_read = int (*)(SSL *, void *, int);
using p_ssl_write = int (*)(SSL *, const void *, int);
using p_ssl_clear = int (*)(SSL *);
using p_ssl_shutdown = int (*)(SSL *);
using p_ssl_get_shutdown = int (*)(SSL *);
using p_ssl_get_error = int (*)(const SSL *, int);
using p_ssl_renegotiate = int (*)(SSL *);
using p_ssl_renegotiate_abbr = int (*)(SSL *);
using p_ssl_renegotiate_pending = int (*)(SSL *);
using p_ssl_do_handshake = int (*)(SSL *);
using p_ssl_get_peer_cert = X509 * (*)(const SSL *);
using p_ssl_get_verify_result = long (*)(const SSL *);
using p_ssl_get_session = SSL_SESSION * (*)(const SSL *);
using p_ssl_set_session = int (*)(SSL *, SSL_SESSION *);
using p_ssl_session_reused = int (*)(SSL *);
using p_ssl_get0_param = X509_VERIFY_PARAM * (*)(SSL *);

using p_ssl_session_d2i = SSL_SESSION * (*)(SSL_SESSION **, const unsigned char **, long);
using p_ssl_session_i2d = int (*)(SSL_SESSION *, unsigned char **);
using p_ssl_session_up_ref = int (*)(SSL_SESSION *);
using p_ssl_session_free = void (*)(SSL_SESSION *);
using p_ssl_session_get_id = const unsigned char * (*)(const SSL_SESSION *, unsigned int *);
using p_ssl_session_get_id_ctx = const unsigned char * (*)(const SSL_SESSION *, unsigned int *);
using p_ssl_session_get_protocol = int (*)(const SSL_SESSION *);
using p_ssl_session_get_time = long (*)(const SSL_SESSION *);
using p_ssl_session_get_timeout = long (*)(const SSL_SESSION *);
using p_ssl_session_set_timeout = long (*)(const SSL_SESSION *, long);
using p_ssl_session_has_ticket = int (*)(const SSL_SESSION *);

using p_x509_verify_param_set_hostflags = void (*)(X509_VERIFY_PARAM *, unsigned int);
using p_x509_verify_param_set1_host = int (*)(X509_VERIFY_PARAM *, const char *, size_t);
using p_x509_verify_param_add1_host = int (*)(X509_VERIFY_PARAM *, const char *, size_t);
using p_x509_verify_param_set1_ip_asc = int (*)(X509_VERIFY_PARAM *, const char *);
using p_x509_verify_cert_error_string = const char * (*)(long);

using p_rand_bytes = int (*)(unsigned char *, int);
using p_evp_sha256 = const EVP_MD * (*)(void);
using p_hmac_init_ex = int (*)(HMAC_CTX *, const void *, int, const EVP_MD *, ENGINE *);
using p_evp_aes_256_cbc = const EVP_CIPHER * (*)(void);
using p_evp_encrypt_init_ex = int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *, const unsigned char *, const unsigned char *);
using p_evp_decrypt_init_ex = int (*)(EVP_CIPHER_CTX *, const EVP_CIPHER *, ENGINE *, const unsigned char *, const unsigned char *);

namespace snf {
namespace ssl {

enum class data_fmt
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
		os << "Non-fatal error - ";
	if (!e.lib.empty())     os << e.lib  << ":";
	if (!e.file.empty())    os << e.file << ":";
	if (!e.fcn.empty())     os << e.fcn  << ":";
	if (e.line != 0)        os << e.line << " ";
	if (!e.data.empty())    os << "[" << e.data << "] ";
	os << e.errstr << "(" << e.error << ")";
	return os;
}

class exception : public std::runtime_error
{
private:
	std::vector<ssl_error> m_error;

	void init();

public:
	exception(const std::string &msg) : std::runtime_error(msg) { init(); }
	exception(const char *msg) : std::runtime_error(msg) { init(); }

	std::vector<ssl_error>::const_iterator begin() const { return m_error.begin(); }
	std::vector<ssl_error>::const_iterator end() const { return m_error.end(); }
};

class ssl_library
{
private:
	snf::dll                        *m_ssl = nullptr;
	snf::dll                        *m_crypto = nullptr;
	std::map<std::string, void *>   m_fcn_table;

	const char *library_name(const std::string &);

	ssl_library();

	void cleanup();

	template<typename T>
	T get_fcn(snf::dll *dll, const std::string &name, bool fatal = true)
	{
		T fcnptr = nullptr;

		std::map<std::string, void *>::iterator it = m_fcn_table.find(name);
		if (it == m_fcn_table.end()) {
			void *ptr = dll->symbol(name.c_str());
			if (ptr)
				m_fcn_table[name] = ptr;
			fcnptr = reinterpret_cast<T>(ptr);
		} else {
			fcnptr = reinterpret_cast<T>(it->second);
		}

		if (!fcnptr && fatal) {
			std::ostringstream oss;
			oss << "symbol " << name << " not found";
			throw std::runtime_error(oss.str());
		}

		return fcnptr;
	}

public:
	static ssl_library &instance()
	{
		static ssl_library ssllib;
		return ssllib;
	}

	~ssl_library() { cleanup(); }

	unsigned long openssl_version(std::string &);
	unsigned long peek_error();

	template<typename T>
	T crypto_fcn(const std::string &name, bool fatal = true)
	{
		return get_fcn<T>(m_crypto, name, fatal);
	}

	template<typename T>
	T ssl_fcn(const std::string &name, bool fatal = true)
	{
		return get_fcn<T>(m_ssl, name, fatal);
	}
};

template<typename T>
T CRYPTO_FCN(const std::string &name, bool fatal = true)
{
	return ssl_library::instance().crypto_fcn<T>(name, fatal);
}

template<typename T>
T SSL_FCN(const std::string &name, bool fatal = true)
{
	return ssl_library::instance().ssl_fcn<T>(name, fatal);
}

inline void
bio_free(BIO *bio)
{
	ssl_library::instance().crypto_fcn<p_bio_free>("BIO_free")(bio);
}

} // namespace ssl
} // namespace snf

#endif // _SNF_SSLFCN_H_
