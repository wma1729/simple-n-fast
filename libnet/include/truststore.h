#ifndef _SNF_TRUSTSTORE_H_
#define _SNF_TRUSTSTORE_H_

#include <string>
#include <vector>
#include "sslfcn.h"
#include "crt.h"
#include "crl.h"

namespace snf {
namespace net {
namespace ssl {

/*
 * Encapsulates OpenSSL X509 trust store (X509_STORE).
 * - A type operator is provided to get the raw trust store.
 * - Certificates and CRLs can be added to the trust store.
 */
class truststore
{
public:
	truststore(const std::string &);
	truststore(X509_STORE *);
	truststore(const truststore &);
	truststore(truststore &&);
	~truststore();

	const truststore &operator=(const truststore &);
	truststore &operator=(truststore &&);

	operator X509_STORE* () { return m_store; }

	void add_certificate(x509_certificate &);
	void add_crl(x509_crl &);

private:
	X509_STORE  *m_store = nullptr;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_TRUSTSTORE_H_
