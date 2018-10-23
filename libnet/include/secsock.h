#ifndef _SNF_SECSOCK_H_
#define _SNF_SECSOCK_H_

#include "sock.h"
#include "sslfcn.h"
#include "ia.h"
#include "ctx.h"
#include <string>
#include <vector>
#include <mutex>

namespace snf {
namespace net {
namespace ssl {

class secure_socket : public snf::net::socket
{
public:
	enum class socket_mode { client, server };

private:
	struct ctxinfo
	{
		bool    cur;
		context ctx;
	};

	struct ssl_error
	{
		bool    want_rd;
		bool    want_wr;
		bool    chk_err_stk;
		int     syserr;
	};

	socket_mode             m_mode;
	std::vector<ctxinfo>    m_contexts;
	std::mutex              m_lock;
	SSL                     *m_ssl = nullptr;

	bool decode_ssl_error(int, ssl_error &);
	void ssl_connect(int);
	void ssl_accept(int);

	secure_socket(sock_t, const sockaddr_storage &, socklen_t, context &, SSL *);

public:
	secure_socket(int, socket_mode, context &);
	secure_socket(const secure_socket &) = delete;
	secure_socket(secure_socket &&);

	const secure_socket &operator=(const secure_socket &) = delete;
	secure_socket &operator=(secure_socket &&);

	bool is_client() const { return (socket_mode::client == m_mode); }
	bool is_server() const { return (socket_mode::server == m_mode); }
	void add_context(context &);
	void switch_context(const std::string &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const internet_address &);
	void set_sni(const std::string &);
	std::string get_sni();
	void enable_sni();
	void connect(int, const std::string &, in_port_t, int to = POLL_WAIT_FOREVER); 
	void connect(const internet_address &, in_port_t, int to = POLL_WAIT_FOREVER);
	void connect(const socket_address &, int to = -1);
	socket accept();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SECSOCK_H_
