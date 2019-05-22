#ifndef _SNF_HTTP_SERVER_H_
#define _SNF_HTTP_SERVER_H_

#include "srvrcfg.h"
#include "sock.h"
#include "cnxn.h"
#include "reactor.h"
#include "thrdpool.h"
#include <memory>

namespace snf {
namespace http {

class server
{
private:
	const server_config                 *m_config = nullptr;
	std::unique_ptr<snf::net::socket>   m_http_sock;
	std::unique_ptr<snf::net::socket>   m_https_sock;
	snf::net::ssl::context              m_ctx;
	snf::net::reactor                   m_reactor;
	std::unique_ptr<snf::thread_pool>   m_thrdpool;
	bool                                m_started = false;
	bool                                m_stopped = false;

	server() {}

	int setup_context();
	snf::net::socket *setup_socket(in_port_t);

public:
	server(const server &) = delete;
	server(server &&) = delete;

	const server &operator=(const server &) = delete;
	server &operator=(server &&) = delete;

	~server() { stop(); }

	static server &instance()
	{
		static server srvr;
		return srvr;
	}

	int start(const server_config *);
	int stop();
	snf::net::ssl::context &ssl_context() { return m_ctx; }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_SERVER_H_
