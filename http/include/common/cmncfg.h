#ifndef _SNF_HTTP_CMN_CONFIG_H_
#define _SNF_HTTP_CMN_CONFIG_H_

#include "net.h"
#include "sslfcn.h"
#include <string>

namespace snf {
namespace http {

/*
 * Common HTTP configuration.
 */
class common_config
{
private:
	in_port_t               m_http_port = 80;       // default HTTP port
	in_port_t               m_https_port = 443;     // default HTTPS port

	std::string             m_keyfile;              // private key file
	std::string             m_kf_pwd;               // private key file password
	snf::ssl::data_fmt      m_kf_fmt = snf::ssl::data_fmt::pem;
	                                                // private key file format

	std::string             m_certfile;             // certificate file
	snf::ssl::data_fmt      m_cf_fmt = snf::ssl::data_fmt::pem;
	                                                // certificate file format

	std::string             m_cafile;               // certificate authority file

	int                     m_cert_chain_depth = 2; // certificate chain depth

public:
	common_config() {}
	virtual ~common_config() {}

	in_port_t http_port() const { return m_http_port; }
	void http_port(in_port_t p) { m_http_port = p; }

	in_port_t https_port() const { return m_https_port; }
	void https_port(in_port_t p) { m_https_port = p; }

	const std::string &keyfile() const { return m_keyfile; }
	void keyfile(const std::string &keyfile) { m_keyfile = keyfile; }
	void keyfile(std::string &&keyfile) { m_keyfile = std::move(keyfile); }

	const std::string &keyfile_password() const { return m_kf_pwd; }
	void keyfile_password(const std::string &kf_pwd) { m_kf_pwd = kf_pwd; }
	void keyfile_password(std::string &&kf_pwd) { m_kf_pwd = std::move(kf_pwd); }

	snf::ssl::data_fmt keyfile_format() const { return m_kf_fmt; }
	void keyfile_format(snf::ssl::data_fmt kf_fmt) { m_kf_fmt = kf_fmt; }

	const std::string &certfile() const { return m_certfile; }
	void certfile(const std::string & certfile) { m_certfile = certfile; }
	void certfile(std::string && certfile) { m_certfile = std::move(certfile); }

	snf::ssl::data_fmt certfile_format() const { return m_cf_fmt; }
	void certfile_format(snf::ssl::data_fmt cf_fmt) { m_cf_fmt = cf_fmt; }

	const std::string &cafile() const { return m_cafile; }
	void cafile(const std::string &cafile) { m_cafile = cafile; }
	void cafile(std::string &&cafile) { m_cafile = std::move(cafile); }

	int certificate_chain_depth() const { return m_cert_chain_depth; }
	void certificate_chain_depth(int depth) { m_cert_chain_depth = depth; }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_CONFIG_H_
