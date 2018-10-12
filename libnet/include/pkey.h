#ifndef _SNF_PKEY_H_
#define _SNF_PKEY_H_

#include <cstdint>
#include "file.h"
#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

class pkey
{
public:
	pkey(ssl_data_fmt, const std::string &, const char *passwd = nullptr);
	pkey(ssl_data_fmt, const uint8_t *, size_t, const char *passwd = nullptr);
	pkey(EVP_PKEY *);
	pkey(const pkey &);
	pkey(pkey &&);
	~pkey();

	const pkey &operator=(const pkey &);
	pkey &operator=(pkey &&);

	/* Return EVP_PKEY_XXX */
	int type() const;
	void verify() const;

	operator EVP_PKEY* () { return m_pkey; }

private:
	EVP_PKEY        *m_pkey = nullptr;

	void init_der(snf::file_ptr &);
	void init_der(const uint8_t *, size_t);
	void init_pem(snf::file_ptr &, const char *);
	void init_pem(const uint8_t *, size_t, const char *);
	void verify_rsa() const;
	void verify_dh() const;
	void verify_ec() const;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_PKEY_H_