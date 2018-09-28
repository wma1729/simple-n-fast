#ifndef _SNF_CRT_H_
#define _SNF_CRT_H_

#include <string>
#include "file.h"
#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

class x509_certificate
{
public:
	x509_certificate(ssl_data_fmt, const std::string &, const char *passwd = nullptr);
	x509_certificate(ssl_data_fmt, const uint8_t *, size_t, const char *passwd = nullptr);
	x509_certificate(X509 *);
	x509_certificate(const x509_certificate &);
	x509_certificate(x509_certificate &&);
	~x509_certificate();

	const x509_certificate &operator=(const x509_certificate &);
	x509_certificate &operator=(x509_certificate &&);

	operator X509* () { return m_crt; }

private:
	X509    *m_crt = nullptr;

	void init_der(snf::file_ptr &);
	void init_der(const uint8_t *, size_t);
	void init_pem(snf::file_ptr &, const char *);
	void init_pem(const uint8_t *, size_t, const char *);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CRT_H_
