#ifndef _SNF_CONNECTION_H_
#define _SNF_CONNECTION_H_

#include "sslfcn.h"
#include "sock.h"
#include "ia.h"
#include "ctx.h"
#include "nio.h"
#include "crt.h"
#include <string>
#include <vector>
#include <mutex>

namespace snf {
namespace net {
namespace ssl {

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

	int handle_ssl_error(sock_t, int, int, const std::string &, int *oserr = 0);
	void ssl_connect(int);
	void ssl_accept(int);

public:
	connection(connection_mode, context &);
	connection(const connection &);
	connection(connection &&);
	~connection();

	const connection &operator=(const connection &);
	connection &operator=(connection &&);

	bool is_client() const { return (connection_mode::client == m_mode); }
	bool is_server() const { return (connection_mode::server == m_mode); }
	void add_context(context &);
	void switch_context(const std::string &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const internet_address &);
	void set_sni(const std::string &);
	std::string get_sni();
	void enable_sni();
	void handshake(const socket &, int to = POLL_WAIT_FOREVER);
	int read(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int write(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	void shutdown();
	void reset();
	void renegotiate(int to = POLL_WAIT_FOREVER);
	x509_certificate *get_peer_certificate();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_CONNECTION_H_
