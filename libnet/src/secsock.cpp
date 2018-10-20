#include "secsock.h"
#include <algorithm>

extern "C" int
sni_cb(SSL *ssl, int *, void *arg)
{
	snf::net::ssl::secure_socket *ssock =
		reinterpret_cast<snf::net::ssl::secure_socket *>(arg);
	if (ssock == nullptr)
		return 0;
	try {
		std::string servername = std::move(ssock->get_sni());
		ssock->switch_context(servername);
	} catch (snf::net::ssl::ssl_exception &) {
		return 0;
	}

	return 1;
}

namespace snf {
namespace net {
namespace ssl {

secure_socket::secure_socket(int family, socket_mode m, context &ctx)
	: socket(family, socket_type::tcp)
	, m_mode(m)
{
	m_ssl = ssl_library::instance().ssl_new()(ctx);
	if (m_ssl == nullptr)
		throw ssl_exception("failed to create SSL object");

	ctxinfo ci;
	ci.cur = true;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

void
secure_socket::add_context(context &ctx)
{
	if (socket_mode::client == m_mode)
		throw ssl_exception("only 1 SSL context can be added in client mode");

	std::lock_guard<std::mutex> guard(m_lock);
	ctxinfo ci;
	ci.cur = false;
	ci.ctx = ctx;
	m_contexts.push_back(ci);
}

void
secure_socket::switch_context(const std::string &servername)
{
	std::lock_guard<std::mutex> guard(m_lock);

	std::vector<ctxinfo>::iterator it;
	for (it = m_contexts.begin(); it != m_contexts.end(); ++it) {
		x509_certificate cert = std::move(it->ctx.get_certificate());
		if (cert.matches(servername)) {
			if (it->cur) {
				// already current
				return;
			} else {
				// make this current
				break;
			}
		}
	}

	if (it == m_contexts.end()) {
		std::ostringstream oss;
		oss << "SSL context for server name " << servername << " not found";
		throw ssl_exception(oss.str());
	} else {
		std::for_each(m_contexts.begin(), m_contexts.end(),
			[](ctxinfo &ci) { ci.cur = false; });
		ssl_library::instance().ssl_set_ssl_ctx()(m_ssl, it->ctx);
		it->cur = true;
	}
}

void
secure_socket::check_hosts(const std::vector<std::string> &hostnames)
{
	int retval;

	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw ssl_exception("failed to get X509 verify parameters");

	std::vector<std::string>::const_iterator it = hostnames.begin();
	if (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_set1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to set " << *it << " to X509 verify parameters";
			throw ssl_exception(oss.str());
		}
		it++;
	}

	while (it != hostnames.end()) {
		retval = ssl_library::instance().x509_verify_param_add1_host()
			(param, it->c_str(), it->size());
		if (retval != 1) {
			std::ostringstream oss;
			oss << "failed to add " << *it << " to X509 verify parameters";
			throw ssl_exception(oss.str());
		}
		it++;
	}

	unsigned int flags =
		X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT |
		X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS;
			
	ssl_library::instance().x509_verify_param_set_hostflags()
		(param, flags);
}

void
secure_socket::check_inaddr(const internet_address &ia)
{
	X509_VERIFY_PARAM *param = ssl_library::instance().ssl_get0_param()(m_ssl);
	if (param == nullptr)
		throw ssl_exception("failed to get X509 verify parameters");

	int retval = ssl_library::instance().x509_verify_param_set1_ip_asc()
		(param, ia.str(false).c_str());
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set " << ia << " to X509 verify parameters";
		throw ssl_exception(oss.str());
	}
}

void
secure_socket::set_sni(const std::string &servername)
{
	if (socket_mode::server == m_mode)
		throw ssl_exception("setting server name for SNI only possible in client mode");

	int retval = ssl_library::instance().ssl_ctrl()
			(m_ssl,
			SSL_CTRL_SET_TLSEXT_HOSTNAME,
			TLSEXT_NAMETYPE_host_name,
			const_cast<char *>(servername.c_str()));
	if (retval != 1) {
		std::ostringstream oss;
		oss << "failed to set SNI for " << servername << " in SSL";
		throw ssl_exception(oss.str());
	}
}

std::string
secure_socket::get_sni()
{
	if (socket_mode::client == m_mode)
		throw ssl_exception("getting server name for SNI only possible in server mode");

	const char *name = ssl_library::instance().ssl_get_servername()
				(m_ssl, TLSEXT_NAMETYPE_host_name);
	if ((name == nullptr) || (*name == '\0'))
		throw ssl_exception("failed to get SNI from SSL");

	std::string servername(name);
	return servername;
}

void
secure_socket::enable_sni()
{
	if (socket_mode::client == m_mode)
		throw ssl_exception("handling SNI requests can only be enabled in server mode");

	std::lock_guard<std::mutex> guard(m_lock);

	std::vector<ctxinfo>::iterator it;
	for (it = m_contexts.begin(); it != m_contexts.end(); ++it) {
		if (ssl_library::instance().ssl_ctx_cb_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB,
			reinterpret_cast<void (*)(void)>(sni_cb)) != 1)
				throw ssl_exception("failed to set SNI callback function");
		if (ssl_library::instance().ssl_ctx_ctrl()
			(it->ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG, 0,
			reinterpret_cast<void *>(this)) != 1)
				throw ssl_exception("failed to set SNI callback argument");
	}
}

} // namespace ssl
} // namespace net
} // namespace snf
