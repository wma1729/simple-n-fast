#ifndef _SNF_REACTOR_H_
#define _SNF_REACTOR_H_

#include <chrono>
#include "netplat.h"
#include "sock.h"
#include <array>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <future>

namespace snf {
namespace net {

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

// Events.
enum class event : short
{
	/*
	 * The following two events can be registered with the reactor.
	 */
	read = POLLRD,
	write = POLLWR,

	/*
	 * Following events can only be returned to the registered
	 * handler. They can not be registered with the reactor.
	 */
	error = POLLERR,
	hup = POLLHUP,         
	invalid = POLLNVAL,
	timeout = POLLTO
};

inline std::string
eventstr(event e)
{
	switch (e) {
		case event::read: return "ev-read";
		case event::write: return "ev-write";
		case event::error: return "ev-error";
		case event::hup: return "ev-hup";
		case event::invalid: return "ev-invalid";
		case event::timeout: return "ev-timeout";
		default: return "ev-unknown";
	}
}

/*
 * Registered socket event handler. This is a pure interface.
 * It must be overridden to provide the event handling.
 */
class handler
{
public:
	virtual ~handler() {}

	/*
	 * Called when the event is triggered.
	 *
	 * @param [in] s - socket ID that received the event.
	 * @param [in] e - event type received.
	 *
	 * @return true to indicate automatic re-registration of the
	 *         handler, false to not to register the handler
	 *         again.
	 */
	virtual bool operator()(sock_t s, event e) = 0;
};

class sockpair_handler;

class reactor
{
private:
	struct ev_info
	{
		event                                   e;   // event
		std::unique_ptr<handler>                h;   // handler
		std::chrono::milliseconds               to;  // timeout
		std::chrono::system_clock::time_point   exp; // expiration

		ev_info(event _e, int _to, handler *_h)
			: e(_e)
			, h(_h)
		{
			if (_to <= 0) {
				to = std::chrono::milliseconds::zero();
				exp = std::chrono::system_clock::time_point::max();
			} else {
				to = std::chrono::milliseconds{_to};
				exp = std::chrono::system_clock::now();
				exp += to;
			}
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

	std::vector<pollfd> set_poll_vector();
	void process_poll_vector(std::vector<pollfd> &);
	void start();

public:
	reactor(int to = 5000);
	reactor(const reactor &) = delete;
	reactor(reactor &&) = delete;
	const reactor &operator=(const reactor &) = delete;
	reactor &operator=(reactor &&) = delete;
	~reactor() { stop(); m_future.wait(); }

	void stop();
	void add_handler(sock_t, event, handler *, int to = 0);
	void remove_handler(sock_t);
	void remove_handler(sock_t, event);
};

} // namespace net
} // namespace snf

#endif // _SNF_REACTOR_H_
