#include "cipher.h"

namespace snf {
namespace ssl {

cipher::cipher(const std::string &name)
{
	m_cipher = CRYPTO_FCN<p_evp_cipher_byname>("EVP_get_cipherbyname")(name.c_str());
	if (m_cipher == nullptr) {
		std::ostringstream oss;
		oss << "failed to get cipher for " << name;
		throw exception(oss.str());
	}
}

size_t
cipher::block_size() const
{
	return CRYPTO_FCN<p_evp_cipher_blksize>("EVP_CIPHER_block_size")(m_cipher);
}

size_t
cipher::key_length() const
{
	return CRYPTO_FCN<p_evp_cipher_keylen>("EVP_CIPHER_key_length")(m_cipher);
}

size_t
cipher::iv_length() const
{
	return CRYPTO_FCN<p_evp_cipher_ivlen>("EVP_CIPHER_iv_length")(m_cipher);
}

} // namespace ssl
} // namespace snf
