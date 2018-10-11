#ifndef _SNF_SSLCTX_H_
#define _SNF_SSLCTX_H_

#include "sslfcn.h"
#include "pkey.h"
#include "crt.h"
#include "truststore.h"

namespace snf {
namespace net {
namespace ssl {

class sslctx {
private:
	SSL_CTX *m_ctx = nullptr;

	long get_options();
	long clr_options(unsigned long);
	long set_options(unsigned long);
public:
	sslctx();
	sslctx(const sslctx &);
	sslctx(sslctx &&);
	const sslctx & operator=(const sslctx &);
	sslctx & operator=(sslctx &&);
	~sslctx();

	void prefer_server_cipher();
	void prefer_client_cipher();
	void new_session_for_renegotiation(bool);
	void tickets_for_session_resumption(bool);
	void set_ciphers(const std::string &ciphers = "TLSv1.2:SSLv3:!aNULL:!eNULL:!aGOST:!MD5:!MEDIUM:!CAMELLIA:!PSK:!RC4::@STRENGTH");
	void use_private_key(pkey &);
	void use_certificate(x509_certificate &);
	void use_truststore(truststore &);
	void check_private_key();
	void verify_peer(bool require_certificate = false, bool do_it_once = false);
	void limit_certificate_chain_depth(int);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SSLCTX_H_
