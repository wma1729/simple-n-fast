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

using p_library_init = int (*)(void);
using p_load_error_strings = void (*)(void);
using p_free_error_strings = void (*)(void);

using p_bio_new_mem_buf = BIO * (*)(const void *, int);
using p_bio_free = int (*)(BIO *);

using p_d2i_private_key_fp = EVP_PKEY * (*)(FILE *, EVP_PKEY **);
using p_d2i_auto_private_key = EVP_PKEY * (*)(EVP_PKEY **, const unsigned char **, long);
using p_pem_read_private_key = EVP_PKEY * (*)(FILE *, EVP_PKEY **, pem_password_cb *, void *);
using p_pem_read_bio_private_key = EVP_PKEY * (*)(BIO *, EVP_PKEY **, pem_password_cb *, void *);
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

using ustring = std::basic_string<uint8_t>;

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

	p_bio_new_mem_buf           m_bio_new_mem_buf = nullptr;
	p_bio_free                  m_bio_free = nullptr;

	p_d2i_private_key_fp        m_d2i_private_key_fp = nullptr;
	p_d2i_auto_private_key      m_d2i_auto_private_key = nullptr;
	p_pem_read_private_key      m_pem_read_private_key = nullptr;
	p_pem_read_bio_private_key  m_pem_read_bio_private_key = nullptr;
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

	p_err_line_data             m_err_line_data = nullptr;
	p_err_lib_string            m_err_lib_string = nullptr;
	p_err_fcn_string            m_err_fcn_string = nullptr;
	p_err_reason_string         m_err_reason_string = nullptr;
	p_err_clear                 m_err_clear = nullptr;

	ssl_library();

public:
	static ssl_library &instance()
	{
		static ssl_library ssllib;
		return ssllib;
	}

	~ssl_library();

	p_library_init library_init();
	p_load_error_strings load_error_strings();
	p_free_error_strings free_error_strings();

	p_bio_new_mem_buf bio_new_mem_buf();
	p_bio_free bio_free();

	p_d2i_private_key_fp d2i_private_key_fp();
	p_d2i_auto_private_key d2i_auto_private_key();
	p_pem_read_private_key pem_read_private_key();
	p_pem_read_bio_private_key pem_read_bio_private_key();
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
