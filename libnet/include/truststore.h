#ifndef _SNF_TRUSTSTORE_H_
#define _SNF_TRUSTSTORE_H_

#include <string>
#include <vector>
#include "sslfcn.h"
#include "crt.h"

namespace snf {
namespace net {
namespace ssl {

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

private:
	X509_STORE  *m_store = nullptr;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_TRUSTSTORE_H_
