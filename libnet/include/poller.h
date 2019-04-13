#ifndef _SNF_POLLER_H_
#define _SNF_POLLER_H_

#include "netplat.h"
#include "sock.h"
#include <array>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>
#include <future>

namespace snf {
namespace net {
namespace poller {

#if defined(_WIN32)
constexpr short POLLRD = POLLRDNORM;
constexpr short POLLWR = POLLWRNORM;
#else
constexpr short POLLRD = POLLIN;
constexpr short POLLWR = POLLOUT;
#endif


/*
 * Following events can be registered with the reactor.
 */
enum class event : short
{
	read = POLLRD,
	write = POLLWR,
	// Can only be returned to handler; cannot be requested
	error = POLLERR,
	hup = POLLHUP,         
	invalid = POLLNVAL
};

class reactor
{
public:
	using handler = std::function<void(sock_t, event)>;

private:
	struct ev_info
	{
		event   e;      // events
		handler h;      // handler
		bool    o;      // run only once
	};

	using ev_info_type    = std::vector<ev_info>;
	using ev_handler_type = std::map<sock_t, ev_info_type>;

	int                               m_timeout;
	std::atomic<bool>                 m_stopped { false };
	std::array<snf::net::socket, 2>   m_sockpair;
	std::future<void>                 m_future;
	std::mutex                        m_lock;
	ev_handler_type                   m_handlers;

public:
	reactor(int to = 5000);
	reactor(const reactor &) = delete;
	reactor(reactor &&) = delete;
	const reactor &operator=(const reactor &) = delete;
	reactor &operator=(reactor &&) = delete;
	~reactor() { stop(); }

	void start();
	void stop();
	void add_handler(sock_t, event, handler, bool one_shot = false);
	void remove_handler(sock_t);
	void remove_handler(sock_t, event);


};

} // namespace poller
} // namespace net
} // namespace snf

#endif // _SNF_POLLER_H_
