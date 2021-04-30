#include "digest.h"

namespace snf {
namespace ssl {

digest::digest(const std::string &name)
{
	m_dgst = CRYPTO_FCN<p_evp_md_digestbyname>("EVP_get_digestbyname")(name.c_str());
	if (m_dgst == nullptr) {
		std::ostringstream oss;
		oss << "failed to get message digest for " << name;
		throw exception(oss.str());
	}

	m_ctx = CRYPTO_FCN<p_evp_md_ctx_new>("EVP_MD_CTX_new")();
	if (m_ctx == nullptr)
		throw exception("failed to allocate message digest context");

	if (CRYPTO_FCN<p_evp_md_dgst_init>("EVP_DigestInit_ex")(m_ctx, m_dgst, nullptr) != 1)
		throw exception("failed to initialize message digest");
}

digest::digest(const digest &d)
{
	m_dgst = d.m_dgst;

	m_ctx = CRYPTO_FCN<p_evp_md_ctx_new>("EVP_MD_CTX_new")();
	if (m_ctx == nullptr)
		throw exception("failed to allocate message digest context");

	if (CRYPTO_FCN<p_evp_md_dgst_init>("EVP_DigestInit_ex")(m_ctx, m_dgst, nullptr) != 1)
		throw exception("failed to initialize message digest");

	if (CRYPTO_FCN<p_evp_md_ctx_copy>("EVP_MD_CTX_copy_ex")(m_ctx, d.m_ctx) != 1)
		throw exception("failed to copy message digest");
}

digest::digest(digest &&d)
{
	m_dgst = d.m_dgst; d.m_dgst = nullptr;
	m_ctx = d.m_ctx;   d.m_ctx = nullptr;
}

digest::~digest()
{
	CRYPTO_FCN<p_evp_md_ctx_free>("EVP_MD_CTX_free")(m_ctx);
}

digest &
digest::operator=(const digest &d)
{
	if (this != &d) {
		m_dgst = d.m_dgst;

		CRYPTO_FCN<p_evp_md_ctx_free>("EVP_MD_CTX_free")(m_ctx);

		m_ctx = CRYPTO_FCN<p_evp_md_ctx_new>("EVP_MD_CTX_new")();
		if (m_ctx == nullptr)
			throw exception("failed to allocate message digest context");

		if (CRYPTO_FCN<p_evp_md_dgst_init>("EVP_DigestInit_ex")(m_ctx, m_dgst, nullptr) != 1)
			throw exception("failed to initialize message digest");

		if (CRYPTO_FCN<p_evp_md_ctx_copy>("EVP_MD_CTX_copy_ex")(m_ctx, d.m_ctx) != 1)
			throw exception("failed to copy message digest");
	}

	return *this;
}

digest &
digest::operator=(digest &&d)
{
	if (this != &d) {
		m_dgst = d.m_dgst; d.m_dgst = nullptr;
		m_ctx = d.m_ctx;   d.m_ctx = nullptr;
	}

	return *this;
}

void
digest::add(const void *buf, size_t len)
{
	if (CRYPTO_FCN<p_evp_md_dgst_update>("EVP_DigestUpdate")(m_ctx, buf, len) != 1)
		throw exception("failed to add data to message digest");
}

safestr *
digest::get()
{
	size_t dgstlen = length();
	safestr *ss = DBG_NEW safestr(dgstlen);
	unsigned int n = 0;

	if (CRYPTO_FCN<p_evp_md_dgst_final>("EVP_DigestFinal_ex")(m_ctx, ss->data(), &n) != 1)
		throw exception("failed to extract message digest");

	if (n != dgstlen) {
		std::ostringstream oss;
		oss << "expected message digest length " << dgstlen << "; got " << n;
		throw exception(oss.str());
	}

	return ss;
}


size_t
digest::length()
{
	return CRYPTO_FCN<p_evp_md_size>("EVP_MD_size")(m_dgst);
}

} // namespace ssl
} // namespace snf
