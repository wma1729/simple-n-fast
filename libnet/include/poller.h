#ifndef _SNF_POLLER_H_
#define _SNF_POLLER_H_

#include "netplat.h"
#include "sock.h"
#include <array>
#include <vector>
#include <map>
#include <memory>
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

/*
 * Flag     Linux  Windows
 * POLLRD   0x0001 0x0100
 * POLLRW   0x0004 0x0200
 * POLLERR  0x0008 0x0001
 * POLLHUP  0x0010 0x0002
 * POLLNVAL 0x0020 0x0004
 */

class reactor
{
private:
	struct ev_info
	{
		event                               e;      // events
		std::function<bool(sock_t, event)>  h;      // handler

		ev_info(event ee, std::function<bool(sock_t, event)> &&hh)
			: e(ee), h(std::move(hh)) {}
		ev_info(const ev_info &ei)
			: e(ei.e), h(ei.h) {}
		ev_info(ev_info &&ei)
			: e(ei.e), h(std::move(ei.h)) {}

		const ev_info &operator=(const ev_info &ei)
		{
			if (this != &ei) {
				e = ei.e;
				h = ei.h;
			}
			return *this;
		}

		ev_info &operator=(ev_info &&ei)
		{
			if (this != &ei) {
				e = ei.e;
				h = std::move(ei.h);
			}
			return *this;
		}
	};

	using ev_info_type    = std::vector<std::unique_ptr<ev_info>>;
	using ev_handler_type = std::map<sock_t, ev_info_type>;

	int                               m_timeout;
	std::atomic<bool>                 m_stopped { false };
	std::array<snf::net::socket, 2>   m_sockpair;
	std::future<void>                 m_future;
	std::mutex                        m_lock;
	ev_handler_type                   m_handlers;

	void set_poll_vector(std::vector<pollfd> &);
	void process_poll_vector(std::vector<pollfd> &, int);
	void start();
	void stop();

public:
	reactor(int to = 5000);
	reactor(const reactor &) = delete;
	reactor(reactor &&) = delete;
	const reactor &operator=(const reactor &) = delete;
	reactor &operator=(reactor &&) = delete;
	~reactor() { stop(); }

	void add_handler(sock_t, event, std::function<bool(sock_t, event)>);
	void remove_handler(sock_t);
	void remove_handler(sock_t, event);
};

} // namespace poller
} // namespace net
} // namespace snf

#endif // _SNF_POLLER_H_
