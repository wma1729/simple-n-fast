#include "truststore.h"

namespace snf {
namespace ssl {

/*
 * Constructs the trust store from the given file.
 *
 * @param [in] f - the trust store (certificate chain) file name.
 *
 * @throws snf::ssl::exception if the trust store could not be created or
 *         the contents of the file could not be loaded.
 */
truststore::truststore(const std::string &f)
{
	m_store = CRYPTO_FCN<p_x509_store_new>("X509_STORE_new")();
	if (m_store == nullptr) {
		throw exception("failed to create X509 trust store");
	} else {
		if (CRYPTO_FCN<p_x509_store_load>("X509_STORE_load_locations")(m_store, f.c_str(), nullptr) != 1) {
			CRYPTO_FCN<p_x509_store_free>("X509_STORE_free")(m_store);
			std::ostringstream oss;
			oss << "failed to load certificates from " << f
				<< " into X509 trust store";
			throw exception(oss.str());
		}
	}
}

/*
 * Constructs the trust store from the raw X509_STORE.
 * It bumps up the reference count of the trust store.
 *
 * @param [in] store - raw X509_STORE..
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
truststore::truststore(X509_STORE *store)
{
	if (CRYPTO_FCN<p_x509_store_up_ref>("X509_STORE_up_ref")(store) != 1)
		throw exception("failed to increment the X509 trust store reference count");
	m_store = store;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw trust store and the reference count in bumped up.
 *
 * @param [in] store - trust store.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
truststore::truststore(const truststore &store)
{
	if (CRYPTO_FCN<p_x509_store_up_ref>("X509_STORE_up_ref")(store.m_store) != 1)
		throw exception("failed to increment the X509 trust store reference count");
	m_store = store.m_store;
}

/*
 * Move constructore.
 */
truststore::truststore(truststore &&store)
{
	m_store = store.m_store;
	store.m_store = nullptr;
}

/*
 * Destructor. The reference count to the trust store is decremented. If it is the
 * last reference, the trust store is deleted.
 */
truststore::~truststore()
{
	if (m_store) {
		CRYPTO_FCN<p_x509_store_free>("X509_STORE_free")(m_store);
		m_store = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw trust store and the reference count in bumped up.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
const truststore &
truststore::operator=(const truststore &store)
{
	if (this != &store) {
		if (CRYPTO_FCN<p_x509_store_up_ref>("X509_STORE_up_ref")(store.m_store) != 1)
			throw exception("failed to increment the X509 trust store reference count");
		if (m_store)
			CRYPTO_FCN<p_x509_store_free>("X509_STORE_free")(m_store);
		m_store = store.m_store;
	}
	return *this;
}

/*
 * Move operator.
 */
truststore &
truststore::operator=(truststore &&store)
{
	if (this != &store) {
		if (m_store)
			CRYPTO_FCN<p_x509_store_free>("X509_STORE_free")(m_store);
		m_store = store.m_store;
		store.m_store = nullptr;
	}
	return *this;
}

/*
 * Adds a certificate to the trust store.
 *
 * @param [in] crt - the certificate to add to the trust store.
 *
 * @throws snf::ssl::exception if the certificate could not be added
 *         to the trust store.
 */
void
truststore::add_certificate(x509_certificate &crt)
{
	if (CRYPTO_FCN<p_x509_store_add_cert>("X509_STORE_add_cert")(m_store, crt) != 1)
		throw exception("failed to add certificate to the X509 trust store");
}

/*
 * Adds CRL to the trust store. The trust store flags X509_V_FLAG_CRL_CHECK &
 * X509_V_FLAG_CRL_CHECK_ALL are set as well.
 *
 * @param [in] crl - the CRL.
 *
 * @throws snf::ssl::exception if the CRL could not be added to the
 *         trust store or the CRL check flags could not be set.
 */
void
truststore::add_crl(x509_crl &crl)
{
	if (CRYPTO_FCN<p_x509_store_add_crl>("X509_STORE_add_crl")(m_store, crl) != 1)
		throw exception("failed to add CRL to the X509 trust store");

	unsigned long flags = X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL;
	if (CRYPTO_FCN<p_x509_store_set_flags>("X509_STORE_set_flags")(m_store, flags) != 1)
		throw exception("failed to set flags for the X509 trust store");
}

} // namespace ssl
} // namespace snf
