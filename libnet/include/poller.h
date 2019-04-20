#ifndef _SNF_POLLER_H_
#define _SNF_POLLER_H_

#include <chrono>
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

/*
 * Flag     Linux  Windows
 * POLLRD   0x0001 0x0100
 * POLLRW   0x0004 0x0200
 * POLLERR  0x0008 0x0001
 * POLLHUP  0x0010 0x0002
 * POLLNVAL 0x0020 0x0004
 */

#if defined(_WIN32)
constexpr short POLLRD = POLLRDNORM;
constexpr short POLLWR = POLLWRNORM;
#else
constexpr short POLLRD = POLLIN;
constexpr short POLLWR = POLLOUT;
#endif

constexpr short POLLTO = 0x1000;

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
	invalid = POLLNVAL,
	timeout = POLLTO
};

class reactor
{
private:
	struct ev_info
	{
		event                                   e;   // events
		std::function<bool(sock_t, event)>      h;   // handler
		std::chrono::milliseconds               to;  // timeout
		std::chrono::system_clock::time_point   exp; // expiration

		ev_info(event _e, int _to, std::function<bool(sock_t, event)> &&_h)
			: e(_e)
			, h(std::move(_h))
		{
			if (_to <= 0) {
#if defined(_WIN32)
#undef max
#endif
				to = std::chrono::milliseconds::zero();
				exp = std::chrono::system_clock::time_point::max();
			} else {
				to = std::chrono::milliseconds{_to};
				exp = std::chrono::system_clock::now();
				exp += to;
			}
		}

		ev_info(const ev_info &ei)
			: e(ei.e)
			, h(ei.h)
			, to(ei.to)
			, exp(ei.exp)
		{
		}

		ev_info(ev_info &&ei)
			: e(ei.e)
			, h(std::move(ei.h))
			, to(ei.to)
			, exp(ei.exp)
		{
		}

		const ev_info &operator=(const ev_info &ei)
		{
			if (this != &ei) {
				e = ei.e;
				h = ei.h;
				to = ei.to;
				exp = ei.exp;
			}
			return *this;
		}

		ev_info &operator=(ev_info &&ei)
		{
			if (this != &ei) {
				e = ei.e;
				h = std::move(ei.h);
				to = ei.to;
				exp = ei.exp;
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
	void process_poll_vector(std::vector<pollfd> &);
	void start();
	void stop();

public:
	reactor(int to = 5000);
	reactor(const reactor &) = delete;
	reactor(reactor &&) = delete;
	const reactor &operator=(const reactor &) = delete;
	reactor &operator=(reactor &&) = delete;
	~reactor() { stop(); }

	void add_handler(sock_t, event, std::function<bool(sock_t, event)>, int to = 0);
	void remove_handler(sock_t);
	void remove_handler(sock_t, event);
};

} // namespace poller
} // namespace net
} // namespace snf

#endif // _SNF_POLLER_H_
