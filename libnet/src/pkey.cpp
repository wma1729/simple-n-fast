#include "file.h"
#include "pkey.h"
#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

void
private_key::init_der(snf::file_ptr &fp)
{
	m_pkey = ssl_library::instance().d2i_private_key_fp()(fp, nullptr);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read DER key from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

void
private_key::init_pem(snf::file_ptr &fp, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);
	m_pkey = ssl_library::instance().pem_read_privatekey()(fp, nullptr, nullptr, pwd);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read PEM key from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

private_key::private_key(private_key::key_fmt fmt, const std::string &kfile, const char *passwd)
{
	file_ptr fp(kfile, "rb");
	if (fmt == private_key::key_fmt::pem) {
		init_pem(fp, passwd);
	} else /* if (fmt == private_key::key_fmt::der) */ {
		init_der(fp);
	}

	if (ssl_library::instance().evp_pkey_base_id()(m_pkey) != EVP_PKEY_RSA) {
		ssl_library::instance().evp_pkey_free()(m_pkey);
		m_pkey = nullptr;

		std::ostringstream oss;
		oss << "key read from file " << kfile << " is not an RSA key";
		throw ssl_exception(oss.str());
	}
}

private_key::~private_key()
{
	if (m_pkey) {
		ssl_library::instance().evp_pkey_free()(m_pkey);
		m_pkey = nullptr;
	}
}

} // namespace ssl
} // namespace net
} // namespace snf
