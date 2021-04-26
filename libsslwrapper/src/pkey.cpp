#include "pkey.h"
#include <memory>

namespace snf {
namespace ssl {

/*
 * Initialize key in der format from file pointer.
 *
 * @param [in] fp - pointer to the file containing the key.
 *
 * @throws snf::ssl::exception if the key could not be read or
 *         the key could not be converted to EVP_PKEY.
 */
void
pkey::init_der(snf::file_ptr &fp)
{
	m_pkey = CRYPTO_FCN<p_d2i_private_key_fp>("d2i_PrivateKey_fp")(fp, nullptr);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read DER key from file " << fp.name();
		throw exception(oss.str());
	}
}

/*
 * Initialize key in der format from memory.
 *
 * @param [in] key    - key.
 * @param [in] keylen - key length.
 *
 * @throws snf::ssl::exception if the key could not converted to EVP_PKEY.
 */
void
pkey::init_der(const uint8_t *key, size_t keylen)
{
	const uint8_t *kptr = key;

	m_pkey = CRYPTO_FCN<p_d2i_auto_private_key>("d2i_AutoPrivateKey")
			(nullptr, &kptr, static_cast<long>(keylen));
	if ((m_pkey == nullptr) || (kptr != (key + keylen)))
		throw exception("failed to load DER key");
}

/*
 * Initialize key in pem format from file pointer.
 *
 * @param [in] fp     - pointer to the file containing the key.
 * @param [in] passwd - password to the pem key.
 *
 * @throws snf::ssl::exception if the key could not be read or
 *         the key could not be converted to EVP_PKEY or
 *         the key password is incorrect.
 */
void
pkey::init_pem(snf::file_ptr &fp, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	m_pkey = CRYPTO_FCN<p_pem_read_private_key>("PEM_read_PrivateKey")
			(fp, nullptr, nullptr, pwd);
	if (m_pkey == nullptr) {
		std::ostringstream oss;
		oss << "failed to read PEM key from file " << fp.name();
		throw exception(oss.str());
	}
}

/*
 * Initialize key in pem format from memory.
 *
 * @param [in] key    - key.
 * @param [in] keylen - key length.
 * @param [in] passwd - password to the pem key.
 *
 * @throws snf::ssl::exception if the key could not converted to EVP_PKEY or
 *         the key password is incorrect.
 */
void
pkey::init_pem(const uint8_t *key, size_t keylen, const char *passwd)
{
	char *pwd = const_cast<char *>(passwd);

	std::unique_ptr<BIO, decltype(&bio_free)> pkbio {
		CRYPTO_FCN<p_bio_new_mem_buf>("BIO_new_mem_buf")(key, static_cast<int>(keylen)),
		bio_free
	};

	m_pkey = CRYPTO_FCN<p_pem_read_bio_private_key>("PEM_read_bio_PrivateKey")
		(pkbio.get(), nullptr, nullptr, pwd);
	if (m_pkey == nullptr)
		throw exception("failed to load PEM key");
}

/*
 * Verifies RSA key.
 *
 * @throws snf::ssl::exception if the key is not a RSA key or
 *         the key is invalid.
 */
void
pkey::verify_rsa() const
{
	RSA *rsa = CRYPTO_FCN<p_get1_rsa>("EVP_PKEY_get1_RSA")(m_pkey);
	if (rsa) {
		int r = CRYPTO_FCN<p_rsa_key_check>("RSA_check_key")(rsa);
		CRYPTO_FCN<p_rsa_free>("RSA_free")(rsa);
		if (1 != r) {
			throw exception("RSA key is not valid");
		}
	} else {
		throw exception("RSA key could not be extracted from private key");
	}
}

/*
 * Verifies DH key.
 *
 * @throws snf::ssl::exception if the key is not a DH key or
 *         the key is invalid.
 */
void
pkey::verify_dh() const
{
	DH *dh = CRYPTO_FCN<p_get1_dh>("EVP_PKEY_get1_DH")(m_pkey);
	if (dh) {
		int codes = 0;
		int r = CRYPTO_FCN<p_dh_key_check>("DH_check")(dh, &codes);
		CRYPTO_FCN<p_dh_free>("DH_free")(dh);
		if (r == 1) {
			if (codes != 0) {
				std::ostringstream oss;
				oss << "DH key is not valid; codes = 0x" << std::hex << codes;
				throw exception(oss.str());
			}
		} else {
			throw exception("DH key is not valid");
		}
	} else {
		throw exception("DH key could not be extracted from private key");
	}
}

/*
 * Verifies EC key.
 *
 * @throws snf::ssl::exception if the key is not an EC key or
 *         the key is invalid.
 */
void
pkey::verify_ec() const
{
	EC_KEY *eckey = CRYPTO_FCN<p_get1_ec_key>("EVP_PKEY_get1_EC_KEY")(m_pkey);
	if (eckey) {
		int r = CRYPTO_FCN<p_ec_key_check>("EC_KEY_check_key")(eckey);
		CRYPTO_FCN<p_ec_key_free>("EC_KEY_free")(eckey);
		if (1 != r) {
			throw exception("EC key is not valid");
		}
	} else {
		throw exception("EC key could not be extracted from private key");
	}
}

/*
 * Construct the key.
 *
 * @param [in] fmt    - key format: der or pem.
 * @param [in] kfile  - key file path.
 * @param [in] passwd - password to the pem key. The password is
 *                      applicable only to pem format.
 *
 * @throws snf::ssl::exception if the key could not be read or
 *         the key could not be converted to EVP_PKEY or
 *         the key password is incorrect.
 */
pkey::pkey(
	data_fmt fmt,
	const std::string &kfile,
	const char *passwd)
{
	snf::file_ptr fp(kfile, "rb");

	if (fmt == data_fmt::pem) {
		init_pem(fp, passwd);
	} else /* if (fmt == data_fmt::der) */ {
		init_der(fp);
	}
}

/*
 * Construct the key.
 *
 * @param [in] fmt    - key format: der or pem.
 * @param [in] key    - key.
 * @param [in] keylen - key length.
 * @param [in] passwd - password to the pem key. The password is
 *                      applicable only to pem format.
 *
 * @throws snf::ssl::exception if the key could not converted to EVP_PKEY or
 *         the key password is incorrect.
 */
pkey::pkey(
	data_fmt fmt,
	const uint8_t *key,
	size_t keylen,
	const char *passwd)
{
	if (fmt == data_fmt::pem) {
		init_pem(key, keylen, passwd);
	} else /* if (fmt == data_fmt::der) */ {
		init_der(key, keylen);
	}
}

/*
 * Constructs the key from the raw EVP_PKEY.
 * It bumps up the reference count of the key.
 *
 * @param [in] key - raw key.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
pkey::pkey(EVP_PKEY *key)
{
	if (CRYPTO_FCN<p_evp_pkey_up_ref>("EVP_PKEY_up_ref")(key) != 1)
		throw exception("failed to increment the key reference count");
	m_pkey = key;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw key and the reference count in bumped up.
 *
 * @param [in] key - key.
 *
 * @throws snf::ssl::exception if the reference count could not be incremented.
 */
pkey::pkey(const pkey &key)
{
	if (CRYPTO_FCN<p_evp_pkey_up_ref>("EVP_PKEY_up_ref")(key.m_pkey) != 1)
		throw exception("failed to increment the key reference count");
	m_pkey = key.m_pkey;
}

/*
 * Move constructor.
 */
pkey::pkey(pkey &&pkey)
{
	m_pkey = pkey.m_pkey;
	pkey.m_pkey = nullptr;
}

/*
 * Destructor. The reference count to the key is decremented. If it is the
 * last reference, the key is deleted.
 */
pkey::~pkey()
{
	if (m_pkey) {
		CRYPTO_FCN<p_evp_pkey_free>("EVP_PKEY_free")(m_pkey);
		m_pkey = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw key and the reference count in bumped up.
 *
 * @throws snf:::ssl::exception if the reference count could not be incremented.
 */
const pkey &
pkey::operator=(const pkey &pkey)
{
	if (this != &pkey) {
		if (CRYPTO_FCN<p_evp_pkey_up_ref>("EVP_PKEY_up_ref")(pkey.m_pkey) != 1)
			throw exception("failed to increment the key reference count");
		if (m_pkey)
			CRYPTO_FCN<p_evp_pkey_free>("EVP_PKEY_free")(m_pkey);
		m_pkey = pkey.m_pkey;
	}
	return *this;
}

/*
 * Move operator.
 */
pkey &
pkey::operator=(pkey &&pkey)
{
	if (this != &pkey) {
		if (m_pkey)
			CRYPTO_FCN<p_evp_pkey_free>("EVP_PKEY_free")(m_pkey);
		m_pkey = pkey.m_pkey;
		pkey.m_pkey = nullptr;
	}
	return *this;
}

/*
 * Get the key type (EVP_PKEY_XXX).
 */
int
pkey::type() const
{
	int kt = EVP_PKEY_NONE;
	if (m_pkey)
		kt = CRYPTO_FCN<p_evp_pkey_base_id>("EVP_PKEY_base_id")(m_pkey);
	return kt;
}

/*
 * Verifies the key.
 *
 * @throws snf::ssl::exception if the key is invalid.
 */
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
			throw exception("invalid key type");
			break;
	}
}

} // namespace ssl
} // namespace snf
