#ifndef _SNF_CRL_H_
#define _SNF_CRL_H_

#include <cstdint>
#include "file.h"
#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

class x509_crl
{
public:
	x509_crl(const std::string &, const char *passwd = nullptr);
	x509_crl(const uint8_t *, size_t, const char *passwd = nullptr);
	x509_crl(X509_CRL *);
	x509_crl(const x509_crl &);
	x509_crl(x509_crl &&);
	~x509_crl();

	const x509_crl &operator=(const x509_crl &);
	x509_crl &operator=(x509_crl &&);

	operator X509_CRL * () { return m_crl; }

private:
	X509_CRL    *m_crl = nullptr;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CRL_H_
