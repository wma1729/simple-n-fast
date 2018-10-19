#include "pkey.h"

namespace snf {
namespace net {
namespace ssl {

void
pkey::init_der(snf::file_ptr &fp)
{
	m_pkey = ssl_library::instance().d2i_private_key_fp()(fp, nullptr);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read DER key from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

void
pkey::init_der(const uint8_t *key, size_t keylen)
{
	const uint8_t *kptr = key;

	m_pkey = ssl_library::instance().d2i_auto_private_key()
			(nullptr, &kptr, static_cast<long>(keylen));
	if ((m_pkey == nullptr) || (kptr != (key + keylen)))
		throw ssl_exception("failed to load DER key");
}

void
pkey::init_pem(snf::file_ptr &fp, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	m_pkey = ssl_library::instance().pem_read_private_key()(fp, nullptr, nullptr, pwd);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read PEM key from file " << fp.name();
		throw ssl_exception(oss.str());
	}
}

void
pkey::init_pem(const uint8_t *key, size_t keylen, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	BIO *pkbio = ssl_library::instance().bio_new_mem_buf()(key, static_cast<int>(keylen));
	m_pkey = ssl_library::instance().pem_read_bio_private_key()(pkbio, nullptr, nullptr, pwd);
	if (m_pkey == nullptr) {
		ssl_library::instance().bio_free()(pkbio);
		throw ssl_exception("failed to load PEM key");
	}
	ssl_library::instance().bio_free()(pkbio);
}

void
pkey::verify_rsa() const
{
	RSA *rsa = ssl_library::instance().get1_rsa()(m_pkey);
	if (rsa) {
		int r = ssl_library::instance().rsa_key_check()(rsa);
		ssl_library::instance().rsa_free()(rsa);
		if (1 != r) {
			throw ssl_exception("RSA key is not valid");
		}
	} else {
		throw ssl_exception("RSA key could not be extracted from private key");
	}
}

void
pkey::verify_dh() const
{
	DH *dh = ssl_library::instance().get1_dh()(m_pkey);
	if (dh) {
		int codes = 0;
		int r = ssl_library::instance().dh_key_check()(dh, &codes);
		ssl_library::instance().dh_free()(dh);
		if (r == 1) {
			if (codes != 0) {
				std::ostringstream oss;
				oss << "DH key is not valid; codes = 0x" << std::hex << codes;
				throw ssl_exception(oss.str());
			}
		} else {
			throw ssl_exception("DH key is not valid");
		}
	} else {
		throw ssl_exception("DH key could not be extracted from private key");
	}
}

void
pkey::verify_ec() const
{
	EC_KEY *eckey = ssl_library::instance().get1_ec_key()(m_pkey);
	if (eckey) {
		int r = ssl_library::instance().ec_key_check()(eckey);
		ssl_library::instance().ec_key_free()(eckey);
		if (1 != r) {
			throw ssl_exception("EC key is not valid");
		}
	} else {
		throw ssl_exception("EC key could not be extracted from private key");
	}
}

pkey::pkey(
	ssl_data_fmt fmt,
	const std::string &kfile,
	const char *passwd)
{
	snf::file_ptr fp(kfile, "rb");

	if (fmt == ssl_data_fmt::pem) {
		init_pem(fp, passwd);
	} else /* if (fmt == ssl_data_fmt::der) */ {
		init_der(fp);
	}
}

pkey::pkey(
	ssl_data_fmt fmt,
	const uint8_t *key,
	size_t keylen,
	const char *passwd)
{
	if (fmt == ssl_data_fmt::pem) {
		init_pem(key, keylen, passwd);
	} else /* if (fmt == ssl_data_fmt::der) */ {
		init_der(key, keylen);
	}
}

pkey::pkey(EVP_PKEY *pkey)
{
	if (ssl_library::instance().evp_pkey_up_ref()(pkey) != 1)
		throw ssl_exception("failed to increment the key reference count");
	m_pkey = pkey;
}

pkey::pkey(const pkey &pkey)
{
	if (ssl_library::instance().evp_pkey_up_ref()(pkey.m_pkey) != 1)
		throw ssl_exception("failed to increment the key reference count");
	m_pkey = pkey.m_pkey;
}

pkey::pkey(pkey &&pkey)
{
	m_pkey = pkey.m_pkey;
	pkey.m_pkey = nullptr;
}


pkey::~pkey()
{
	if (m_pkey) {
		ssl_library::instance().evp_pkey_free()(m_pkey);
		m_pkey = nullptr;
	}
}

const pkey &
pkey::operator=(const pkey &pkey)
{
	if (this != &pkey) {
		if (ssl_library::instance().evp_pkey_up_ref()(pkey.m_pkey) != 1)
			throw ssl_exception("failed to increment the key reference count");
		if (m_pkey)
			ssl_library::instance().evp_pkey_free()(m_pkey);
		m_pkey = pkey.m_pkey;
	}
	return *this;
}

pkey &
pkey::operator=(pkey &&pkey)
{
	if (this != &pkey) {
		if (m_pkey)
			ssl_library::instance().evp_pkey_free()(m_pkey);
		m_pkey = pkey.m_pkey;
		pkey.m_pkey = nullptr;
	}
	return *this;
}

int
pkey::type() const
{
	int kt = EVP_PKEY_NONE;
	if (m_pkey)
		kt = ssl_library::instance().evp_pkey_base_id()(m_pkey);
	return kt;
}

void
pkey::verify() const
{
	int kt = type();
	switch (kt) {
		case EVP_PKEY_RSA:
		case EVP_PKEY_RSA2:
			verify_rsa();
			break;

		case EVP_PKEY_DSA:
		case EVP_PKEY_DSA1:
		case EVP_PKEY_DSA2:
		case EVP_PKEY_DSA3:
		case EVP_PKEY_DSA4:
			// assume the key is valid
			break;

		case EVP_PKEY_DH:
			verify_dh();
			break;

		case EVP_PKEY_EC:
			verify_ec();
			break;

		default:
			throw ssl_exception("invalid key type");
			break;
	}
}

} // namespace ssl
} // namespace net
} // namespace snf
