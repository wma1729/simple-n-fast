#include "crt.h"
#include <iostream>

namespace snf {
namespace net {
namespace ssl {

void
x509_certificate::init_der(snf::file_ptr &fp)
{
	m_crt = ssl_library::instance().d2i_x509_fp()(fp, nullptr);
	if (m_crt == nullptr) {
		std::ostringstream oss;
		oss << "failed to read DER certificate from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

void
x509_certificate::init_der(const uint8_t *crt, size_t crtlen)
{
	const uint8_t *cptr = crt;

	m_crt = ssl_library::instance().d2i_x509()
			(nullptr, &cptr, static_cast<long>(crtlen));
	if ((m_crt == nullptr) || (cptr != (crt + crtlen)))
		throw ssl_exception("failed to load DER certificate");
}

void
x509_certificate::init_pem(snf::file_ptr &fp, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	m_crt = ssl_library::instance().pem_read_x509()(fp, nullptr, nullptr, pwd);
	if (m_crt == nullptr) {
		std::ostringstream oss;
		oss << "failed to read PEM certificate from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

void
x509_certificate::init_pem(const uint8_t *crt, size_t crtlen, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	BIO *cbio = ssl_library::instance().bio_new_mem_buf()(crt, static_cast<int>(crtlen));
	m_crt = ssl_library::instance().pem_read_bio_x509()(cbio, nullptr, nullptr, pwd);
	if (m_crt == nullptr) {
		ssl_library::instance().bio_free()(cbio);
		throw ssl_exception("failed to load PEM certificate");
	}
	ssl_library::instance().bio_free()(cbio);
}

x509_certificate::x509_certificate(
	ssl_data_fmt fmt,
	const std::string &cfile,
	const char *passwd)
{
	snf::file_ptr fp(cfile, "rb");

	if (fmt == ssl_data_fmt::pem) {
		init_pem(fp, passwd);
	} else /* if (fmt == ssl_data_fmt::der) */ {
		init_der(fp);
	}
}

x509_certificate::x509_certificate(
	ssl_data_fmt fmt,
	const uint8_t *crt,
	size_t crtlen,
	const char *passwd)
{
	if (fmt == ssl_data_fmt::pem) {
		init_pem(crt, crtlen, passwd);
	} else /* if (fmt == ssl_data_fmt::der) */ {
		init_der(crt, crtlen);
	}
}

x509_certificate::x509_certificate(X509 *crt)
{
	if (ssl_library::instance().x509_up_ref()(crt) != 1)
		throw ssl_exception("failed to increment the certificate reference count");
	m_crt = crt;
}

x509_certificate::x509_certificate(const x509_certificate &crt)
{
	if (ssl_library::instance().x509_up_ref()(crt.m_crt) != 1)
		throw ssl_exception("failed to increment the certificate reference count");
	m_crt = crt.m_crt;
}

x509_certificate::x509_certificate(x509_certificate &&crt)
{
	m_crt = crt.m_crt;
	crt.m_crt = nullptr;
}


x509_certificate::~x509_certificate()
{
	if (m_crt) {
		ssl_library::instance().x509_free()(m_crt);
		m_crt = nullptr;
	}
}

const x509_certificate &
x509_certificate::operator=(const x509_certificate &crt)
{
	if (this != &crt) {
		if (ssl_library::instance().x509_up_ref()(crt.m_crt) != 1)
			throw ssl_exception("failed to increment the certificate reference count");
		m_crt = crt.m_crt;
	}
	return *this;
}

x509_certificate &
x509_certificate::operator=(x509_certificate &&crt)
{
	if (this != &crt) {
		m_crt = crt.m_crt;
		crt.m_crt = nullptr;
	}
	return *this;
}

} // namespace ssl
} // namespace net
} // namespace snf
