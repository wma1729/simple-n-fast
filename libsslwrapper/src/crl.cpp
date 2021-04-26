#include "crl.h"
#include <memory>

namespace snf {
namespace ssl {

/*
 * Construct CRL object from the content of the file. The data must be in pem format.
 *
 * @param [in] crlfile - CRL file.
 * @param [in] passwd  - password to the CRL.
 *
 * @throws snf::ssl::exception if the CRL could not be read from the file or
 *         the password specified is incorrect.
 */
x509_crl::x509_crl(
	const std::string &crlfile,
	const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);
	snf::file_ptr fp(crlfile, "rb");

	m_crl = CRYPTO_FCN<p_pem_read_x509_crl>("PEM_read_X509_CRL")(fp, nullptr, nullptr, pwd);
	if (m_crl == nullptr) {
		std::ostringstream oss;
		oss << "failed to read X509 CRL from file " << crlfile;
		throw exception(oss.str());
	}
}

/*
 * Construct CRL object from memory. The data must be in pem format.
 *
 * @param [in] crl    - CRL.
 * @param [in] crllen - CRL length.
 * @param [in] passwd - password to the CRL.
 *
 * @throws snf::ssl::exception if the CRL could not loaded or
 *         the password specified is incorrect.
 */
x509_crl::x509_crl(
	const uint8_t *crl,
	size_t crllen,
	const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	std::unique_ptr<BIO, decltype(&bio_free)> crlbio {
		CRYPTO_FCN<p_bio_new_mem_buf>("BIO_new_mem_buf")(crl, static_cast<int>(crllen)),
		bio_free
	};

	m_crl = CRYPTO_FCN<p_pem_read_bio_x509_crl>("PEM_read_X509_CRL")
		(crlbio.get(), nullptr, nullptr, pwd);
	if (m_crl == nullptr)
		throw exception("failed to load X509 CRL");
}

/*
 * Constructs the CRL from the raw X509_crl.
 * It bumps up the reference count of the certificate.
 *
 * @param [in] crl - raw CRL.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
x509_crl::x509_crl(X509_CRL *crl)
{
	if (CRYPTO_FCN<p_x509_crl_up_ref>("X509_CRL_up_ref")(crl) != 1)
		throw exception("failed to increment the X509 CRL reference count");
	m_crl = crl;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw CRL and the reference count in bumped up.
 *
 * @param [in] crl - CRL.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
x509_crl::x509_crl(const x509_crl &crl)
{
	if (CRYPTO_FCN<p_x509_crl_up_ref>("X509_CRL_up_ref")(crl.m_crl) != 1)
		throw exception("failed to increment the X509 CRL reference count");
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
		CRYPTO_FCN<p_x509_crl_free>("X509_CRL_free")(m_crl);
		m_crl = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw CRL and the reference count in bumped up.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
const x509_crl &
x509_crl::operator=(const x509_crl &crl)
{
	if (this != &crl) {
		if (CRYPTO_FCN<p_x509_crl_up_ref>("X509_CRL_up_ref")(crl.m_crl) != 1)
			throw exception("failed to increment the X509 CRL reference count");
		if (m_crl)
			CRYPTO_FCN<p_x509_crl_free>("X509_CRL_free")(m_crl);
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
			CRYPTO_FCN<p_x509_crl_free>("X509_CRL_free")(m_crl);
		m_crl = crl.m_crl;
		crl.m_crl = nullptr;
	}
	return *this;
}

} // namespace ssl
} // namespace snf
