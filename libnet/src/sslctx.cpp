#include "sslctx.h"
#include <sstream>

namespace snf {
namespace net {
namespace ssl {

long
sslctx::get_options()
{
	p_ssl_ctx_get_options pgetopt = ssl_library::instance().ssl_ctx_get_options();
	if (pgetopt != nullptr) {
		return pgetopt(m_ctx);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_OPTIONS, 0, nullptr);
	}
}

long
sslctx::clr_options(unsigned long opt)
{
	p_ssl_ctx_clr_options pclropt = ssl_library::instance().ssl_ctx_clr_options();
	if (pclropt != nullptr) {
		return pclropt(m_ctx, opt);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_CLEAR_OPTIONS, opt, nullptr);
	}
}

long
sslctx::set_options(unsigned long opt)
{
	p_ssl_ctx_set_options psetopt = ssl_library::instance().ssl_ctx_set_options();
	if (psetopt != nullptr) {
		return psetopt(m_ctx, opt);
	} else {
		return ssl_library::instance().ssl_ctx_ctrl()
			(m_ctx, SSL_CTRL_OPTIONS, opt, nullptr);
	}
}

sslctx::sslctx()
{
	m_ctx = ssl_library::instance().ssl_ctx_new()
			(ssl_library::instance().tls_method()());
	if (m_ctx == nullptr)
		throw ssl_exception("failed to create ssl context");

	if (ssl_library::instance().ssl_ctx_set_min_ver()(m_ctx, TLS1_VERSION) != 1) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		throw ssl_exception("failed to set minimum protocol version");
	}

	if (ssl_library::instance().ssl_ctx_set_max_ver()(m_ctx, 0) != 1) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		throw ssl_exception("failed to set maximum protocol version");
	}

	// mimics SSL_CTX_set_mode
	long mode = SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER;
	ssl_library::instance().ssl_ctx_ctrl()
		(m_ctx, SSL_CTRL_MODE, mode, nullptr);
}

sslctx::sslctx(const sslctx &ctx)
{
	if (ssl_library::instance().ssl_ctx_up_ref()(ctx.m_ctx) != 1)
		throw ssl_exception("failed to increment the ssl context reference count");
	m_ctx = ctx.m_ctx;
}

sslctx::sslctx(sslctx &&ctx)
{
	m_ctx = ctx.m_ctx;
	ctx.m_ctx = nullptr;		
}

const sslctx &
sslctx::operator=(const sslctx &ctx)
{
	if (this != &ctx) {
		if (ssl_library::instance().ssl_ctx_up_ref()(ctx.m_ctx) != 1)
			throw ssl_exception("failed to increment the ssl context reference count");
		if (m_ctx)
			ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = ctx.m_ctx;
	}
	return *this;
}

sslctx &
sslctx::operator=(sslctx &&ctx)
{
	if (this != &ctx) {
		if (m_ctx)
			ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = ctx.m_ctx;
		ctx.m_ctx = nullptr;		
	}
	return *this;
}

sslctx::~sslctx()
{
	if (m_ctx) {
		ssl_library::instance().ssl_ctx_free()(m_ctx);
		m_ctx = nullptr;
	}
}

void
sslctx::prefer_server_cipher()
{
	set_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

void
sslctx::prefer_client_cipher()
{
	clr_options(SSL_OP_CIPHER_SERVER_PREFERENCE);
}

void
sslctx::new_session_for_renegotiation(bool v)
{
	if (v)
		set_options(SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
	else
		clr_options(SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);
}

void
sslctx::tickets_for_session_resumption(bool v)
{
	if (v)
		clr_options(SSL_OP_NO_TICKET);
	else
		set_options(SSL_OP_NO_TICKET);
}

void
sslctx::set_ciphers(const std::string &ciphers)
{
	if (ssl_library::instance().ssl_ctx_set_ciphers()(m_ctx, ciphers.c_str()) != 1) {
		std::ostringstream oss;
		oss << "failed to set cipher list using " << ciphers;
		throw ssl_exception(oss.str());
	}
}

void
sslctx::use_private_key(pkey &key)
{
	if (ssl_library::instance().ssl_ctx_use_private_key()(m_ctx, key) != 1)
		throw ssl_exception("failed to use private key");
}

void
sslctx::use_certificate(x509_certificate &crt)
{
	if (ssl_library::instance().ssl_ctx_use_certificate()(m_ctx, crt) != 1)
		throw ssl_exception("failed to use X509 certificate");
}

void
sslctx::use_truststore(truststore &store)
{
	ssl_library::instance().ssl_ctx_use_truststore()(m_ctx, store);
}

void
sslctx::use_crl(x509_crl &crl)
{
	X509_STORE *store = ssl_library::instance().ssl_ctx_get_cert_store()(m_ctx);
	if (store == nullptr)
		throw ssl_exception("failed to get X509 trust store for the current context");

	if (ssl_library::instance().x509_store_add_crl()(store, crl) != 1)
		throw ssl_exception("failed to add CRL to the X509 trust store");
}

void
sslctx::check_private_key()
{
	if (ssl_library::instance().ssl_ctx_check_private_key()(m_ctx) != 1)
		throw ssl_exception("private key and X509 certificate do not match");
}

void
sslctx::verify_peer(bool require_certificate, bool do_it_once)
{
	int mode = SSL_VERIFY_PEER;
	if (require_certificate)
		mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	if (do_it_once)
		mode |= SSL_VERIFY_CLIENT_ONCE;

	ssl_library::instance().ssl_ctx_set_verify()(m_ctx, mode, nullptr);
}

void
sslctx::limit_certificate_chain_depth(int depth)
{
	ssl_library::instance().ssl_ctx_set_verify_depth()(m_ctx, depth);
}

} // namespace ssl
} // namespace net
} // namespace snf