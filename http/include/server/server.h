#ifndef _SNF_HTTP_SERVER_H_
#define _SNF_HTTP_SERVER_H_

#include "srvrcfg.h"
#include "sock.h"
#include "cnxn.h"
#include "reactor.h"
#include "thrdpool.h"
#include "router.h"
#include <memory>

namespace snf {
namespace http {

class server
{
private:
	const server_config                 *m_config = nullptr;
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
	void register_path(const std::string &, request_handler_t);
	snf::net::ssl::context &ssl_context() { return m_ctx; }
	snf::net::reactor &reactor() { return m_reactor; }
	snf::thread_pool *thread_pool() { return m_thrdpool.get(); }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_SERVER_H_
