#include "crypt.h"

namespace snf {
namespace ssl {

crypt::crypt()
{
	m_ctx = CRYPTO_FCN<p_evp_cipher_ctx_new>("EVP_CIPHER_CTX_new")();
	if (m_ctx == nullptr)
		throw exception("failed to allocate crypt context");
}

crypt::~crypt()
{
	CRYPTO_FCN<p_evp_cipher_ctx_free>("EVP_CIPHER_CTX_free")(m_ctx);
}

encrypt::encrypt(const cipher &c, const safestr &key, const safestr &iv)
	: crypt{}
{
	m_blksize = c.block_size();
	const EVP_CIPHER *cphr = c;
	if (CRYPTO_FCN<p_evp_encrypt_init_ex>("EVP_EncryptInit_ex")(m_ctx, cphr, nullptr, key.data(), iv.data()) != 1)
		throw exception("failed to initialize encryption");
}

size_t
encrypt::process(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen, bool last)
{
	int ilen = static_cast<int>(inlen);
	int n = 0;
	size_t olen = 0;
	

	if (CRYPTO_FCN<p_evp_encrypt_update>("EVP_EncryptUpdate")(m_ctx, out, &n, in, ilen) != 1)
		throw exception("failed to encrypt the data");
	olen = n;
	n = 0;

	if (last) {
		if (CRYPTO_FCN<p_evp_encrypt_final_ex>("EVP_EncryptFinal_ex")(m_ctx, out + olen, &n) != 1)
			throw exception("failed to encrypt the last data block");
		olen += n;
		n = 0;
	}

	if (outlen < olen)
		throw std::runtime_error("encryption caused buffer overflow");

	return olen;
}

decrypt::decrypt(const cipher &c, const safestr &key, const safestr &iv)
	: crypt{}
{
	const EVP_CIPHER *cphr = c;
	if (CRYPTO_FCN<p_evp_decrypt_init_ex>("EVP_DecryptInit_ex")(m_ctx, cphr, nullptr, key.data(), iv.data()) != 1)
		throw exception("failed to initialize decryption");
}

size_t
decrypt::process(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen, bool last)
{
	int ilen = static_cast<int>(inlen);
	int n = 0;
	size_t olen = 0;

	if (CRYPTO_FCN<p_evp_decrypt_update>("EVP_DecryptUpdate")(m_ctx, out, &n, in, ilen) != 1)
		throw exception("failed to decrypt the data");
	olen = n;
	n = 0;

	if (last) {
		if (CRYPTO_FCN<p_evp_decrypt_final_ex>("EVP_DecryptFinal_ex")(m_ctx, out + olen, &n) != 1)
			throw exception("failed to decrypt the last data block");
		olen += n;
		n = 0;
	}

	if (outlen < olen)
		throw std::runtime_error("decryption caused buffer overflow");

	return olen;
}

} // namespace ssl
} // namespace snf
