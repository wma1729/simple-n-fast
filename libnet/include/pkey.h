#ifndef _SNF_PKEY_H_
#define _SNF_PKEY_H_

#include <string>
#include <openssl/evp.h>

namespace snf {
namespace net {
namespace ssl {

class private_key
{
public:
	enum class key_fmt
	{
		der,
		pem
	};

	private_key(key_fmt, const std::string &, const char *passwd = nullptr);
	~private_key();

	operator EVP_PKEY* () { return m_pkey; }

private:
	EVP_PKEY    *m_pkey = nullptr;
	key_fmt     m_kfmt;

	void init_der(snf::file_ptr &);
	void init_pem(snf::file_ptr &, const char *);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_PKEY_H_
