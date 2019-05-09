#ifndef _SNF_CONNECTION_H_
#define _SNF_CONNECTION_H_

#include "sslfcn.h"
#include "sock.h"
#include "ia.h"
#include "ctx.h"
#include "nio.h"
#include "crt.h"
#include "session.h"
#include <string>
#include <vector>
#include <mutex>

extern "C" int sni_cb(SSL *, int *, void *);

namespace snf {
namespace net {
namespace ssl {

enum class operation;
struct error_info;

/*
 * Represents secured TLS connection. Manages all aspects of a secured connection.
 * - supports Server Name Identification (SNI): multiple contexts can be added and
 *   context switch occurs transparently.
 * - supports TLS session resumption: session ID context and session ticket based.
 * - supports simple host/ip addr checks.
 */
class connection : public snf::net::nio
{
private:
	struct ctxinfo
	{
		bool    cur;
		context ctx;
	};

	connection_mode         m_mode;
	std::vector<ctxinfo>    m_contexts;
	std::mutex              m_lock;
	SSL                     *m_ssl = nullptr;

	void switch_context(const std::string &);
	std::string get_sni();
	int handle_ssl_error(sock_t, int, error_info &);

public:
	connection(connection_mode, context &);
	connection(const connection &);
	connection(connection &&);
	virtual ~connection();

	const connection &operator=(const connection &);
	connection &operator=(connection &&);

	bool is_client() const { return (connection_mode::client == m_mode); }
	bool is_server() const { return (connection_mode::server == m_mode); }
	void add_context(context &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const internet_address &);
	void set_sni(const std::string &);
	void enable_sni();
	session get_session();
	void set_session(session &);
	bool is_session_reused();
	void handshake(const socket &, int to = POLL_WAIT_FOREVER);
	int readn(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int writen(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	void shutdown();
	void reset();
	x509_certificate *get_peer_certificate();
	bool is_verification_successful(std::string &);

	friend int ::sni_cb(SSL *, int *, void *);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CONNECTION_H_
