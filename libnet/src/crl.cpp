#include "crl.h"
#include <memory>

namespace snf {
namespace net {
namespace ssl {

/*
 * Construct CRL object from the content of the file. The data must be in pem format.
 *
 * @param [in] crlfile - CRL file.
 * @param [in] passwd  - password to the CRL.
 *
 * @throws snf::net::ssl::ssl_exception if the CRL could not be read from the file or
 *         the password specified is incorrect.
 */
x509_crl::x509_crl(
	const std::string &crlfile,
	const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);
	snf::file_ptr fp(crlfile, "rb");

	m_crl = ssl_library::instance().pem_read_x509_crl()(fp, nullptr, nullptr, pwd);
	if (m_crl == nullptr) {
		std::ostringstream oss;
		oss << "failed to read X509 CRL from file " << crlfile;
		throw ssl_exception(oss.str());
	}
}

/*
 * Construct CRL object from memory. The data must be in pem format.
 *
 * @param [in] crl    - CRL.
 * @param [in] crllen - CRL length.
 * @param [in] passwd - password to the CRL.
 *
 * @throws snf::net::ssl::ssl_exception if the CRL could not loaded or
 *         the password specified is incorrect.
 */
x509_crl::x509_crl(
	const uint8_t *crl,
	size_t crllen,
	const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	std::unique_ptr<BIO, decltype(&bio_free)> crlbio {
		ssl_library::instance().bio_new_mem_buf()(crl, static_cast<int>(crllen)),
		bio_free
	};

	m_crl = ssl_library::instance().pem_read_bio_x509_crl()
		(crlbio.get(), nullptr, nullptr, pwd);
	if (m_crl == nullptr)
		throw ssl_exception("failed to load X509 CRL");
}

/*
 * Constructs the CRL from the raw X509_crl.
 * It bumps up the reference count of the certificate.
 *
 * @param [in] crl - raw CRL.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
x509_crl::x509_crl(X509_CRL *crl)
{
	if (ssl_library::instance().x509_crl_up_ref()(crl) != 1)
		throw ssl_exception("failed to increment the X509 CRL reference count");
	m_crl = crl;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw CRL and the reference count in bumped up.
 *
 * @param [in] crl - CRL.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
x509_crl::x509_crl(const x509_crl &crl)
{
	if (ssl_library::instance().x509_crl_up_ref()(crl.m_crl) != 1)
		throw ssl_exception("failed to increment the X509 CRL reference count");
	m_crl = crl.m_crl;
}

/*
 * Move constructor.
 */
x509_crl::x509_crl(x509_crl &&crl)
{
	m_crl = crl.m_crl;
	crl.m_crl = nullptr;
}

/*
 * Destructor. The reference count to the CRL is decremented. If it is the
 * last reference, the CRL is deleted.
 */
x509_crl::~x509_crl()
{
	if (m_crl) {
		ssl_library::instance().x509_crl_free()(m_crl);
		m_crl = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw CRL and the reference count in bumped up.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
const x509_crl &
x509_crl::operator=(const x509_crl &crl)
{
	if (this != &crl) {
		if (ssl_library::instance().x509_crl_up_ref()(crl.m_crl) != 1)
			throw ssl_exception("failed to increment the X509 CRL reference count");
		if (m_crl)
			ssl_library::instance().x509_crl_free()(m_crl);
		m_crl = crl.m_crl;
	}
	return *this;
}

/*
 * Move operator.
 */
x509_crl &
x509_crl::operator=(x509_crl &&crl)
{
	if (this != &crl) {
		if (m_crl)
			ssl_library::instance().x509_crl_free()(m_crl);
		m_crl = crl.m_crl;
		crl.m_crl = nullptr;
	}
	return *this;
}

} // namespace ssl
} // namespace net
} // namespace snf
