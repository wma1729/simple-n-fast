#ifndef _SNF_CRT_H_
#define _SNF_CRT_H_

#include <string>
#include <vector>
#include "file.h"
#include "sslfcn.h"

namespace snf {
namespace net {
namespace ssl {

/*
 * Encapsulates OpenSSL X509 Certificate (X509).
 * - The certificate can be in der or pem format.
 * - A type operator is provided to get the raw certificate.
 */
class x509_certificate
{
public:
	struct altname {
		std::string type;
		std::string name;
	};

	x509_certificate(data_fmt, const std::string &, const char *passwd = nullptr);
	x509_certificate(data_fmt, const uint8_t *, size_t, const char *passwd = nullptr);
	x509_certificate(X509 *);
	x509_certificate(const x509_certificate &);
	x509_certificate(x509_certificate &&);
	~x509_certificate();

	const x509_certificate &operator=(const x509_certificate &);
	x509_certificate &operator=(x509_certificate &&);

	operator X509* () { return m_crt; }

	const std::string &subject();
	const std::string &issuer();
	const std::string &common_name();
	const std::string &serial();
	const std::vector<x509_certificate::altname> &alternate_names();
	const std::vector<std::string> &crl_distribution_points();
	const std::vector<std::string> &ocsp_end_points();
	bool matches(const std::string &);

private:
	X509                        *m_crt = nullptr;
	std::string                 m_subject;
	std::string                 m_issuer;
	std::string                 m_cn;
	std::string                 m_serial;
	std::vector<altname>        m_alt_names;
	std::vector<std::string>    m_crl_dps;
	std::vector<std::string>    m_ocsp_eps;

	void init_der(snf::file_ptr &);
	void init_der(const uint8_t *, size_t);
	void init_pem(snf::file_ptr &, const char *);
	void init_pem(const uint8_t *, size_t, const char *);
	std::string gn_2_str(const GENERAL_NAME *);
	bool equal(const std::string &, const std::string &);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CRT_H_
