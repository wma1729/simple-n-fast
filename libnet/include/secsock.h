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

	secure_socket(int, socket_mode, context &);

	bool is_client() const { return (socket_mode::client == m_mode); }
	bool is_server() const { return (socket_mode::server == m_mode); }
	void add_context(context &);
	void switch_context(const std::string &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const internet_address &);
	void set_sni(const std::string &);
	std::string get_sni();
	void enable_sni();

private:

	struct ctxinfo
	{
		bool    cur;
		context ctx;
	};

	socket_mode             m_mode;
	std::vector<ctxinfo>    m_contexts;
	std::mutex              m_lock;
	SSL                     *m_ssl = nullptr;
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_SECSOCK_H_
