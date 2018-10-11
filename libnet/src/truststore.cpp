#include "truststore.h"

namespace snf {
namespace net {
namespace ssl {

truststore::truststore(const std::string &f)
{
	m_store = ssl_library::instance().x509_store_new()();
	if (m_store == nullptr) {
		throw ssl_exception("failed to create X509 trust store");
	} else {
		if (ssl_library::instance().x509_store_load()(m_store, f.c_str(), nullptr) != 1) {
			ssl_library::instance().x509_store_free()(m_store);
			std::ostringstream oss;
			oss << "failed to load certificates from " << f
				<< " into X509 trust store";
			throw ssl_exception(oss.str());
		}
	}
}

truststore::truststore(X509_STORE *store)
{
	m_store = store;
}

truststore::truststore(const truststore &store)
{
	if (ssl_library::instance().x509_store_up_ref()(store.m_store) != 1)
		throw ssl_exception("failed to increment the X509 trust store reference count");
	m_store = store.m_store;
}

truststore::truststore(truststore &&store)
{
	m_store = store.m_store;
	store.m_store = nullptr;
}

truststore::~truststore()
{
	if (m_store) {
		ssl_library::instance().x509_store_free()(m_store);
		m_store = nullptr;
	}
}

const truststore &
truststore::operator=(const truststore &store)
{
	if (this != &store) {
		if (ssl_library::instance().x509_store_up_ref()(store.m_store) != 1)
			throw ssl_exception("failed to increment the X509 trust store reference count");
		if (m_store)
			ssl_library::instance().x509_store_free()(m_store);
		m_store = store.m_store;
	}
	return *this;
}

truststore &
truststore::operator=(truststore &&store)
{
	if (this != &store) {
		if (m_store)
			ssl_library::instance().x509_store_free()(m_store);
		m_store = store.m_store;
		store.m_store = nullptr;
	}
	return *this;
}

void
truststore::add_certificate(x509_certificate &crt)
{
	if (ssl_library::instance().x509_store_add_cert()(m_store, crt) != 1)
		throw ssl_exception("failed to add certificate to the X509 trust store");
}

} // namespace ssl
} // namespace net
} // namespace snf
