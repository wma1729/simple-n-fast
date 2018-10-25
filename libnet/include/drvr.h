#ifndef _SNF_DRIVER_H_
#define _SNF_DRIVER_H_

#include "sslfcn.h"
#include "sock.h"
#include "ia.h"
#include "ctx.h"
#include <string>
#include <vector>
#include <mutex>

namespace snf {
namespace net {
namespace ssl {

class driver
{
public:
	enum class driver_mode { client, server };

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

	driver_mode             m_mode;
	std::vector<ctxinfo>    m_contexts;
	std::mutex              m_lock;
	SSL                     *m_ssl = nullptr;

	bool decode_ssl_error(int, ssl_error &);
	void ssl_connect(int);
	void ssl_accept(int);

public:
	driver(driver_mode, context &);
	driver(const driver &);
	driver(driver &&);
	~driver();

	const driver &operator=(const driver &);
	driver &operator=(driver &&);

	bool is_client() const { return (driver_mode::client == m_mode); }
	bool is_server() const { return (driver_mode::server == m_mode); }
	void add_context(context &);
	void switch_context(const std::string &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const internet_address &);
	void set_sni(const std::string &);
	std::string get_sni();
	void enable_sni();
	void handshake(const socket &, int to = 120000);
	int read(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int write(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	void shutdown();
	void reset();
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_DRIVER_H_
