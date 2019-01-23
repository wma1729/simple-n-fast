#include "common.h"
#include "crt.h"
#include "ia.h"
#include <regex>
#include <memory>

namespace snf {
namespace net {
namespace ssl {

/*
 * Initialize certificate in der format from file pointer.
 *
 * @param [in] fp - pointer to the file containing the certificate.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not be read or
 *         the certificate could not be converted to X509.
 */
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

/*
 * Initialize certificate in der format from memory.
 *
 * @param [in] crt    - certificate.
 * @param [in] crtlen - certificate length.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not converted to X509.
 */
void
x509_certificate::init_der(const uint8_t *crt, size_t crtlen)
{
	const uint8_t *cptr = crt;

	m_crt = ssl_library::instance().d2i_x509()
			(nullptr, &cptr, static_cast<long>(crtlen));
	if ((m_crt == nullptr) || (cptr != (crt + crtlen)))
		throw ssl_exception("failed to load DER certificate");
}

/*
 * Initialize certificate in pem format from file pointer.
 *
 * @param [in] fp     - pointer to the file containing the certificate.
 * @param [in] passwd - password to the pem certificate.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not be read or
 *         the certificate could not be converted to X509 or
 *         the certificate password is incorrect.
 */
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

/*
 * Initialize certificate in pem format from memory.
 *
 * @param [in] crt    - certificate.
 * @param [in] crtlen - certificate length.
 * @param [in] passwd - password to the pem certificate.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not converted to X509 or
 *         the certificate password is incorrect.
 */
void
x509_certificate::init_pem(const uint8_t *crt, size_t crtlen, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	std::unique_ptr<BIO, decltype(&bio_free)> cbio {
		ssl_library::instance().bio_new_mem_buf()(crt, static_cast<int>(crtlen)),
		bio_free
	};

	m_crt = ssl_library::instance().pem_read_bio_x509()
		(cbio.get(), nullptr, nullptr, pwd);
	if (m_crt == nullptr)
		throw ssl_exception("failed to load PEM certificate");
}

/*
 * String representation of the GENERAL_NAME.
 *
 * @param [in] gn - pointer to the GENERAL_NAME.
 *
 * @return the string representation of the GENERAL_NAME.
 * Only GEN_DNS, GEN_URI, and GEN_IPADD are handled. Empty
 * string is returned for the rest.
 */
std::string
x509_certificate::gn_2_str(const GENERAL_NAME *gn)
{
	std::string s;
	const uint8_t *data = nullptr;
	int len = 0;

	if (gn->type == GEN_DNS) {
		data = ssl_library::instance().asn1_string_val()
			(gn->d.dNSName);
		len = ssl_library::instance().asn1_string_len()
			(gn->d.dNSName);
		s.assign(reinterpret_cast<const char *>(data),
			static_cast<size_t>(len));
	} else if (gn->type == GEN_URI) {
		data = ssl_library::instance().asn1_string_val()
			(gn->d.uniformResourceIdentifier);
		len = ssl_library::instance().asn1_string_len()
			(gn->d.uniformResourceIdentifier);
		s.assign(reinterpret_cast<const char *>(data),
			static_cast<size_t>(len));
	} else if (gn->type == GEN_IPADD) {
		data = ssl_library::instance().asn1_string_val()
			(gn->d.iPAddress);
		len = ssl_library::instance().asn1_string_len()
			(gn->d.iPAddress);
		if (len == 4) {
			const in_addr *ia4 = reinterpret_cast<const in_addr *>(data);
			snf::net::internet_address ia(*ia4);
			s = ia.str(true);
		} else if (len == 16) {
			const in6_addr *ia6 = reinterpret_cast<const in6_addr *>(data);
			snf::net::internet_address ia(*ia6);
			s = ia.str(true);
		}
	}

	return s;
}

/*
 * Determines if two certificate names are same.
 *
 * @param [in] name1 - the first certificate name (no regex allowed).
 * @param [in] name2 - the second certificate name (regex allowed).
 *
 * @return true if the names are same, false otherwise.
 */
bool
x509_certificate::equal(const std::string &name1, const std::string &name2)
{
	if (name2.at(0) == '*') {
		std::regex pat(name2, std::regex::icase);
		return std::regex_match(name1, pat);
	} else {
		return snf::streq(name1, name2, true);
	}
}

/*
 * Construct the certificate.
 *
 * @param [in] fmt    - certificate format: der or pem.
 * @param [in] cfile  - certificate file path.
 * @param [in] passwd - password to the pem certificate. The password is
 *                      applicable only to pem format.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not be read or
 *         the certificate could not be converted to X509 or
 *         the certificate password is incorrect.
 */
x509_certificate::x509_certificate(
	data_fmt fmt,
	const std::string &cfile,
	const char *passwd)
{
	snf::file_ptr fp(cfile, "rb");

	if (fmt == data_fmt::pem) {
		init_pem(fp, passwd);
	} else /* if (fmt == data_fmt::der) */ {
		init_der(fp);
	}
}

/*
 * Construct the certificate.
 *
 * @param [in] fmt    - certificate format: der or pem.
 * @param [in] crt    - certificate.
 * @param [in] crtlen - certificate length.
 * @param [in] passwd - password to the pem certificate. The password is
 *                      applicable only to pem format.
 *
 * @throws snf::net::ssl::ssl_exception if the certificate could not converted to X509 or
 *         the certificate password is incorrect.
 */
x509_certificate::x509_certificate(
	data_fmt fmt,
	const uint8_t *crt,
	size_t crtlen,
	const char *passwd)
{
	if (fmt == data_fmt::pem) {
		init_pem(crt, crtlen, passwd);
	} else /* if (fmt == data_fmt::der) */ {
		init_der(crt, crtlen);
	}
}

/*
 * Constructs the certificate from the raw X509.
 * It bumps up the reference count of the certificate.
 *
 * @param [in] crt - raw certificate.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
x509_certificate::x509_certificate(X509 *crt)
{
	if (ssl_library::instance().x509_up_ref()(crt) != 1)
		throw ssl_exception("failed to increment the certificate reference count");
	m_crt = crt;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw certificate and the reference count in bumped up.
 *
 * @param [in] crt - certificate.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
x509_certificate::x509_certificate(const x509_certificate &crt)
{
	if (ssl_library::instance().x509_up_ref()(crt.m_crt) != 1)
		throw ssl_exception("failed to increment the certificate reference count");
	m_crt = crt.m_crt;
	m_subject = crt.m_subject;
	m_issuer = crt.m_issuer;
	m_cn = crt.m_cn;
	m_alt_names = crt.m_alt_names;
	m_crl_dps = crt.m_crl_dps;
	m_ocsp_eps = crt.m_ocsp_eps;
}

/*
 * Move constructor.
 */
x509_certificate::x509_certificate(x509_certificate &&crt)
{
	m_crt = crt.m_crt;
	crt.m_crt = nullptr;
	m_subject = std::move(crt.m_subject);
	m_issuer = std::move(crt.m_issuer);
	m_cn = std::move(crt.m_cn);
	m_alt_names = std::move(crt.m_alt_names);
	m_crl_dps = std::move(crt.m_crl_dps);
	m_ocsp_eps = std::move(crt.m_ocsp_eps);
}

/*
 * Destructor. The reference count to the certificate is decremented. If it is the
 * last reference, the certificate is deleted.
 */
x509_certificate::~x509_certificate()
{
	if (m_crt) {
		ssl_library::instance().x509_free()(m_crt);
		m_crt = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * certificate and the reference count in bumped up.
 *
 * @throws snf::net::ssl::ssl_exception if the reference count could not be incremented.
 */
const x509_certificate &
x509_certificate::operator=(const x509_certificate &crt)
{
	if (this != &crt) {
		if (ssl_library::instance().x509_up_ref()(crt.m_crt) != 1)
			throw ssl_exception("failed to increment the certificate reference count");
		if (m_crt)
			ssl_library::instance().x509_free()(m_crt);
		m_crt = crt.m_crt;
		m_subject = crt.m_subject;
		m_issuer = crt.m_issuer;
		m_cn = crt.m_cn;
		m_alt_names = crt.m_alt_names;
		m_crl_dps = crt.m_crl_dps;
		m_ocsp_eps = crt.m_ocsp_eps;
	}
	return *this;
}

/*
 * Move operator.
 */
x509_certificate &
x509_certificate::operator=(x509_certificate &&crt)
{
	if (this != &crt) {
		if (m_crt)
			ssl_library::instance().x509_free()(m_crt);
		m_crt = crt.m_crt;
		crt.m_crt = nullptr;
		m_subject = std::move(crt.m_subject);
		m_issuer = std::move(crt.m_issuer);
		m_cn = std::move(crt.m_cn);
		m_alt_names = std::move(crt.m_alt_names);
		m_crl_dps = std::move(crt.m_crl_dps);
		m_ocsp_eps = std::move(crt.m_ocsp_eps);
	}
	return *this;
}

/*
 * Gets the certificate's subject. The common name is also retrieved here.
 *
 * @throws snf::net::ssl::ssl_exception if the subject and/or the common
 *         name could not be retrieved from the certificate.
 */
const std::string &
x509_certificate::subject()
{
	if (!m_subject.empty())
		return m_subject;

	int len;
	char buf[2048];
	X509_NAME *n;

	n = ssl_library::instance().x509_get_subject()(m_crt);
	if (n == nullptr)
		throw ssl_exception("failed to get subject from certificate");

	std::unique_ptr<BIO, decltype(&bio_free)> bio {
		ssl_library::instance().bio_new()(ssl_library::instance().bio_s_mem()()),
		bio_free
	};

	len = ssl_library::instance().x509_name_get()
		(bio.get(), n, 0, XN_FLAG_ONELINE);
	if (len < 0)
		throw ssl_exception("failed to transfer X509 name to bio");

	len = ssl_library::instance().bio_read()
		(bio.get(), buf, static_cast<int>(sizeof(buf)) - 1);
	if (len < 0)
		throw ssl_exception("failed to fetch subject from bio");

	m_subject.insert(0, buf, len);

	len = ssl_library::instance().x509_name_get_text_by_nid()
		(n, NID_commonName, buf, static_cast<int>(sizeof(buf)) - 1);
	if (len > 0) {
		m_cn.insert(0, buf, len);
	}

	return m_subject;
}

/*
 * Gets the certificate's issuer.
 *
 * @throws snf::net::ssl::ssl_exception if the issuer
 *         could not be retrieved from the certificate.
 */
const std::string &
x509_certificate::issuer()
{
	if (!m_issuer.empty())
		return m_issuer;

	int len;
	char buf[2048];
	X509_NAME *n;

	n = ssl_library::instance().x509_get_issuer()(m_crt);
	if (n == nullptr)
		throw ssl_exception("failed to get issuer from certificate");

	std::unique_ptr<BIO, decltype(&bio_free)> bio {
		ssl_library::instance().bio_new()(ssl_library::instance().bio_s_mem()()),
		bio_free
	};

	len = ssl_library::instance().x509_name_get()
		(bio.get(), n, 0, XN_FLAG_ONELINE);
	if (len < 0)
		throw ssl_exception("failed to transfer X509 name to bio");

	len = ssl_library::instance().bio_read()
		(bio.get(), buf, static_cast<int>(sizeof(buf)) - 1);
	if (len < 0)
		throw ssl_exception("failed to fetch issuer from bio");

	m_issuer.insert(0, buf, len);

	return m_issuer;
}

/*
 * Gets the certificate's common name.
 *
 * @throws snf::net::ssl::ssl_exception if the common
 *         name could not be retrieved from the certificate.
 */
const std::string &
x509_certificate::common_name()
{
	if (!m_cn.empty())
		return m_cn;

	subject();

	return m_cn;
}

/*
 * Gets the certificate's serial number.
 *
 * @throws snf::net::ssl::ssl_exception if the serial
 *         number could not be retrieved from the certificate.
 */
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

/*
 * Gets the certificate's alternate names. Only the names of
 * type GEN_DNS, GEN_URI, and GEN_IPADD are handled. No alternate
 * names is not an error.
 *
 * @throws snf::net::ssl::ssl_exception if the alternate
 *         names could not be retrieved from the certificate.
 */
const std::vector<x509_certificate::altname> &
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

		x509_certificate::altname an;

		if (altname->type == GEN_DNS) {
			an.type = "DNS";
			an.name = std::move(gn_2_str(altname));
		} else if (altname->type == GEN_URI) {
			an.type = "URI";
			an.name = std::move(gn_2_str(altname));
		} else if (altname->type == GEN_IPADD) {
			an.type = "IP";
			an.name = std::move(gn_2_str(altname));
		} else {
			continue;
		}

		m_alt_names.push_back(std::move(an));
	}

	ssl_library::instance().stk_deep_free()
		(altname_stack, ssl_library::instance().gen_name_free());

	return m_alt_names;
}

/*
 * Gets the certification revocation list (CRL) distribution points
 * from the certificate. No data is not an error.
 *
 * @throws snf::net::ssl::ssl_exception if the CRL
 *         distribution points could not be retrieved from the certificate.
 */
const std::vector<std::string> &
x509_certificate::crl_distribution_points()
{
	if (!m_crl_dps.empty())
		return m_crl_dps;

	_STACK *crl_dp_stack = nullptr;

	crl_dp_stack = static_cast<_STACK *>(ssl_library::instance().x509_get_ext_d2i()
		(m_crt, NID_crl_distribution_points, nullptr, nullptr));
	if (crl_dp_stack == nullptr) {
		return m_crl_dps;
	}

	int dp_count = ssl_library::instance().stk_num()(crl_dp_stack);
	for (int i = 0; i < dp_count; ++i) {
		DIST_POINT *crl_dp = static_cast<DIST_POINT *>(
			ssl_library::instance().stk_val()(crl_dp_stack, i));

		DIST_POINT_NAME *crl_dpn = crl_dp->distpoint;

		if (crl_dpn == nullptr)
			continue;

		if (crl_dpn->type != 0)
			continue;

		_STACK *gn_stack = reinterpret_cast<_STACK *>(crl_dpn->name.fullname);
		int gn_count = ssl_library::instance().stk_num()(gn_stack);
		for (int j = 0; j < gn_count; ++j) {
			GENERAL_NAME *name = static_cast<GENERAL_NAME *>(
				ssl_library::instance().stk_val()(gn_stack, j));

			if (name->type == GEN_URI) {
				std::string s;
				s = std::move(gn_2_str(name));
				if (!s.empty())
					m_crl_dps.push_back(s);
			}
		}
	}

	ssl_library::instance().crl_dps_free()
		(reinterpret_cast<CRL_DIST_POINTS *>(crl_dp_stack));

	return m_crl_dps;
}

/*
 * Gets the online certificate status protocol (OCSP) end points
 * from the certificate. No data is not an error.
 *
 * @throws snf::net::ssl::ssl_exception if the OCSP
 *         end points could not be retrieved from the certificate.
 */
const std::vector<std::string> &
x509_certificate::ocsp_end_points()
{
	if (!m_ocsp_eps.empty())
		return m_ocsp_eps;

	_STACK *aia_stack = nullptr;

	aia_stack = static_cast<_STACK *>(ssl_library::instance().x509_get_ext_d2i()
		(m_crt, NID_info_access, nullptr, nullptr));
	if (aia_stack == nullptr) {
		return m_ocsp_eps;
	}

	int aia_count = ssl_library::instance().stk_num()(aia_stack);
	for (int i = 0; i < aia_count; ++i) {
		ACCESS_DESCRIPTION *ad = static_cast<ACCESS_DESCRIPTION *>(
			ssl_library::instance().stk_val()(aia_stack, i));

		if (ssl_library::instance().obj2nid()(ad->method) == NID_ad_OCSP) {
			if (ad->location->type == GEN_URI) {
				std::string s;
				s = std::move(gn_2_str(ad->location));
				if (!s.empty())
					m_ocsp_eps.push_back(s);
			}
		}
	}

	ssl_library::instance().aia_free()
		(reinterpret_cast<AUTHORITY_INFO_ACCESS *>(aia_stack));

	return m_ocsp_eps;
}

/*
 * Determines if the given server name matches the certificate.
 * The common name and alternate names, if any, are checked for a match.
 * The server name is compared against the alternate names of type DNS
 * and IP address only.
 *
 * @param [in] servername - the server name to match.
 *
 * @return true if the server name matches the certificate, false otherwise.
 */
bool
x509_certificate::matches(const std::string &servername)
{
	const std::string &name = common_name();

	if (!name.empty())
		if (equal(servername, name))
			return true;

	for (auto &altname : alternate_names())
		if ((altname.type == "DNS") || (altname.type == "IP"))
			if (equal(servername, altname.name))
				return true; 

	return false;
}

} // namespace ssl
} // namespace net
} // namespace snf
