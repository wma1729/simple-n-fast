#ifndef _SNF_CTX_H_
#define _SNF_CTX_H_

#include "sslfcn.h"
#include "net.h"
#include "pkey.h"
#include "crt.h"
#include "crl.h"
#include "truststore.h"
#include "keymgr.h"

namespace snf {
namespace net {
namespace ssl {

#define DEFAULT_CIPHER_LIST \
	"TLSv1.2:SSLv3:!aNULL:!eNULL:!aGOST:!MD5:!MEDIUM:!CAMELLIA:!PSK:!RC4::@STRENGTH"

/*
 * Encapsulates OpenSSL SSL context (SSL_CTX).
 * - A type operator is provided to get the raw SSL context.
 * - Ability to specify various options.
 * - Ability to use private key, trust store, and CRLs.
 */
class context {
private:
	static keymgr   *s_km;
	SSL_CTX         *m_ctx = nullptr;

	long get_options();
	long clr_options(unsigned long);
	long set_options(unsigned long);
	void disable_session_caching();
	x509_certificate get_certificate();

public:
	/*
	 * Gets the key manager. If not already set using
	 * set_keymgr(), a basic key manager is created
	 * and used.
	 */
	static keymgr *get_keymgr()
	{
		if (s_km == nullptr) s_km = DBG_NEW basic_keymgr();
		return s_km;
	}

	/*
	 * Sets the key manager. Be careful about its use. It is not
	 * thread-safe. But most of the time, you would set your key
	 * manager at the start of the program where the risk of
	 * concurrency are little.
	 */
	static void set_keymgr(keymgr *km)
	{
		if (s_km) delete s_km;
		s_km = km;
	}

	context();
	context(const context &);
	context(context &&);
	const context & operator=(const context &);
	context & operator=(context &&);
	~context();

	operator SSL_CTX * () { return m_ctx; }

	void prefer_server_cipher();
	void prefer_client_cipher();
	time_t session_timeout();
	time_t session_timeout(time_t);
	void set_session_context(const std::string &);
	void session_ticket(connection_mode, bool);
	void set_ciphers(const std::string &ciphers = DEFAULT_CIPHER_LIST);
	void use_private_key(pkey &);
	void use_certificate(x509_certificate &);
	void use_truststore(truststore &);
	void use_crl(x509_crl &);
	void check_private_key();
	void verify_peer(bool require_certificate = false, bool do_it_once = false);
	void limit_certificate_chain_depth(int);

	friend class connection;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CTX_H_
