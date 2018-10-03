#include "crt.h"
#include "ia.h"

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
	m_subject = crt.m_subject;
	m_issuer = crt.m_issuer;
	m_cn = crt.m_cn;
	m_alt_names = crt.m_alt_names;
}

x509_certificate::x509_certificate(x509_certificate &&crt)
{
	m_crt = crt.m_crt;
	crt.m_crt = nullptr;
	m_subject = std::move(crt.m_subject);
	m_issuer = std::move(crt.m_issuer);
	m_cn = std::move(crt.m_cn);
	m_alt_names = std::move(crt.m_alt_names);
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
		m_subject = crt.m_subject;
		m_issuer = crt.m_issuer;
		m_cn = crt.m_cn;
		m_alt_names = crt.m_alt_names;
	}
	return *this;
}

x509_certificate &
x509_certificate::operator=(x509_certificate &&crt)
{
	if (this != &crt) {
		m_crt = crt.m_crt;
		crt.m_crt = nullptr;
		m_subject = std::move(crt.m_subject);
		m_issuer = std::move(crt.m_issuer);
		m_cn = std::move(crt.m_cn);
		m_alt_names = std::move(crt.m_alt_names);
	}
	return *this;
}

const std::string &
x509_certificate::subject()
{
	if (!m_subject.empty())
		return m_subject;

	int len;
	char buf[2048];
	BIO *bio;
	X509_NAME *n;

	n = ssl_library::instance().x509_get_subject()(m_crt);
	if (n == nullptr)
		throw ssl_exception("failed to get subject from certificate");

	bio = ssl_library::instance().bio_new()
		(ssl_library::instance().bio_s_mem()());

	len = ssl_library::instance().x509_name_get()
		(bio, n, 0, XN_FLAG_ONELINE);
	if (len < 0) {
		ssl_library::instance().bio_free()(bio);
		throw ssl_exception("failed to transfer X509 name to bio");
	}

	len = ssl_library::instance().bio_read()(bio, buf, static_cast<int>(sizeof(buf)) - 1);
	if (len < 0) {
		ssl_library::instance().bio_free()(bio);
		throw ssl_exception("failed to fetch subject from bio");
	}

	ssl_library::instance().bio_free()(bio);

	m_subject.insert(0, buf, len);

	len = ssl_library::instance().x509_name_get_text_by_nid()
		(n, NID_commonName, buf, static_cast<int>(sizeof(buf)) - 1);
	if (len > 0) {
		m_cn.insert(0, buf, len);
	}

	return m_subject;
}

const std::string &
x509_certificate::issuer()
{
	if (!m_issuer.empty())
		return m_issuer;

	int len;
	char buf[2048];
	BIO *bio;
	X509_NAME *n;

	n = ssl_library::instance().x509_get_issuer()(m_crt);
	if (n == nullptr)
		throw ssl_exception("failed to get issuer from certificate");

	bio = ssl_library::instance().bio_new()
		(ssl_library::instance().bio_s_mem()());

	len = ssl_library::instance().x509_name_get()
		(bio, n, 0, XN_FLAG_ONELINE);
	if (len < 0) {
		ssl_library::instance().bio_free()(bio);
		throw ssl_exception("failed to transfer X509 name to bio");
	}

	len = ssl_library::instance().bio_read()(bio, buf, static_cast<int>(sizeof(buf)) - 1);
	if (len < 0) {
		ssl_library::instance().bio_free()(bio);
		throw ssl_exception("failed to fetch issuer from bio");
	}

	ssl_library::instance().bio_free()(bio);

	m_issuer.insert(0, buf, len);

	return m_issuer;
}

const std::string &
x509_certificate::common_name()
{
	if (!m_cn.empty())
		return m_cn;

	subject();

	return m_cn;
}

const std::string &
x509_certificate::serial()
{
	if (!m_serial.empty())
		return m_serial;

	ASN1_INTEGER *aiserial = ssl_library::instance().x509_get_serial()(m_crt);
	BIGNUM *bnserial = ssl_library::instance().asn1_integer_to_bn()
				(aiserial, nullptr);
	if (bnserial == nullptr) {
		throw ssl_exception("failed to convert ASN1_INTEGER to BIGNUM");
	}

	char *hexserial = ssl_library::instance().bn2hex()(bnserial);
	if (hexserial == nullptr) {
		ssl_library::instance().bn_free()(bnserial);
		throw ssl_exception("failed to convert BIGNUM to hex string");
	}

	m_serial = hexserial;

	free(hexserial);
	ssl_library::instance().bn_free()(bnserial);

	return m_serial;
}

const std::vector<alternate_name> &
x509_certificate::alternate_names()
{
	if (!m_alt_names.empty())
		return m_alt_names;

	_STACK *altname_stack = nullptr;

	altname_stack = static_cast<_STACK *>(ssl_library::instance().x509_get_ext_d2i()
		(m_crt, NID_subject_alt_name, nullptr, nullptr));
	if (altname_stack == nullptr) {
		return m_alt_names;
	}

	int count = ssl_library::instance().stk_num()(altname_stack);
	for (int i = 0; i < count; ++i) {
		GENERAL_NAME *altname = static_cast<GENERAL_NAME *>(
			ssl_library::instance().stk_val()(altname_stack, i));

		alternate_name an;
		const uint8_t *data = nullptr;
		int len = 0;

		if (altname->type == GEN_DNS) {
			an.type = "DNS";
			data = ssl_library::instance().asn1_string_val()
				(altname->d.dNSName);
			len = ssl_library::instance().asn1_string_len()
				(altname->d.dNSName);
			an.name.assign(reinterpret_cast<const char *>(data),
				static_cast<size_t>(len));
		} else if (altname->type == GEN_URI) {
			an.type = "URI";
			data = ssl_library::instance().asn1_string_val()
				(altname->d.uniformResourceIdentifier);
			len = ssl_library::instance().asn1_string_len()
				(altname->d.uniformResourceIdentifier);
			an.name.assign(reinterpret_cast<const char *>(data),
				static_cast<size_t>(len));
		} else if (altname->type == GEN_IPADD) {
			an.type = "IP";
			data = ssl_library::instance().asn1_string_val()
				(altname->d.iPAddress);
			len = ssl_library::instance().asn1_string_len()
				(altname->d.iPAddress);
			if (len == 4) {
				const in_addr *ia4 = reinterpret_cast<const in_addr *>(data);
				snf::net::internet_address ia(*ia4);
				an.name = ia.str(true);
			} else if (len == 16) {
				const in6_addr *ia6 = reinterpret_cast<const in6_addr *>(data);
				snf::net::internet_address ia(*ia6);
				an.name = ia.str(true);
			} else {
				continue;
			}
		} else {
			continue;
		}

		m_alt_names.push_back(std::move(an));
	}

	ssl_library::instance().stk_deep_free()
		(altname_stack, ssl_library::instance().gen_name_free());

	return m_alt_names;
}

} // namespace ssl
} // namespace net
} // namespace snf
