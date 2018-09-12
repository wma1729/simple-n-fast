#ifndef _SNF_SSLFCN_H_
#define _SNF_SSLFCN_H_

#include "dll.h"
#include <stdexcept>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

using p_library_init = int (*)(void);
using p_load_error_strings = void (*)(void);
using p_free_error_strings = void (*)(void);
using p_d2i_private_key_fp = EVP_PKEY * (*)(FILE *, EVP_PKEY **);
using p_pem_read_privatekey = EVP_PKEY * (*)(FILE *, EVP_PKEY **, pem_password_cb *, void *);
using p_evp_pkey_base_id = int (*)(EVP_PKEY *);
using p_evp_pkey_free = void (*)(EVP_PKEY *);

namespace snf {
namespace net {
namespace ssl {

class ssl_exception : public std::runtime_error
{
public:
	ssl_exception(const std::string &msg) : std::runtime_error(msg) { }
	ssl_exception(const char *msg) : std::runtime_error(msg) { }
};

class ssl_library
{
private:
	snf::dll                *m_ssl = nullptr;
	p_library_init          m_library_init = nullptr;
	p_load_error_strings    m_load_error_strings = nullptr;
	p_free_error_strings    m_free_error_strings = nullptr;
	p_d2i_private_key_fp    m_d2i_private_key_fp = nullptr;
	p_pem_read_privatekey   m_pem_read_privatekey = nullptr;
	p_evp_pkey_base_id      m_evp_pkey_base_id = nullptr;
	p_evp_pkey_free         m_evp_pkey_free = nullptr;

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
	p_d2i_private_key_fp d2i_private_key_fp();
	p_pem_read_privatekey pem_read_privatekey();
	p_evp_pkey_base_id evp_pkey_base_id();
	p_evp_pkey_free evp_pkey_free();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SSLFCN_H_
