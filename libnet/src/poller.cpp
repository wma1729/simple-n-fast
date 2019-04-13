#include "net.h"
#include "poller.h"

namespace snf {
namespace net {
namespace poller {

reactor::reactor(int to)
	: m_timeout(to)
	, m_sockpair(std::move(snf::net::socket::socketpair()))
{
	m_sockpair[0].blocking(false);
	sock_t s = m_sockpair[0];

	add_handler(s, event::read, [this] (sock_t s, event e) {
		int dummy = 0;
		this->m_sockpair[0].read_integral(&dummy, POLL_WAIT_NONE);
	});

	m_future = std::async(std::launch::async, &reactor::start, this);
}

void
reactor::start()
{
	while (!m_stopped) {
		std::vector<pollfd> poll_vec;

		{
			std::lock_guard<std::mutex> guard(m_lock);
			for (auto &h : m_handlers) {
				pollfd fdelem = { h.first, 0, 0 };
				for (auto &ei : h.second)
					fdelem.events |= static_cast<short>(ei.e);
				poll_vec.push_back(fdelem);
			}
		}

		
		int syserr;
		int retval = snf::net::poll(poll_vec, m_timeout, &syserr);
		if (SOCKET_ERROR == retval) {
			std::ostringstream oss;
			oss << "poll failed";
			throw std::system_error(
				syserr,
				std::system_category(),
				oss.str());
		} else if (retval > 0) {
			for (auto &elem : poll_vec) {
				if (retval && (elem.revents != 0)) {
					std::lock_guard<std::mutex> guard(m_lock);

					ev_handler_type::iterator H;
					H = m_handlers.find(elem.fd);
					if (H != m_handlers.end()) {
						for (auto &ei : H->second) {
							short e = static_cast<short>(ei.e);
							if ((ei.e == event::read) && (elem.revents & e)) {
								ei.h(elem.fd, ei.e);
								elem.revents &= ~e;
							} else if ((ei.e == event::write) && (elem.revents & e)) {
								ei.h(elem.fd, ei.e);
								elem.revents &= ~POLLOUT;
							}

							if (elem.revents & POLLHUP)
								ei.h(elem.fd, event::hup);
							else if (elem.revents & POLLNVAL)
								ei.h(elem.fd, event::invalid);
							else if (elem.revents != 0)
								ei.h(elem.fd, event::error);
						}
						retval--;
					}
				}
			}
		}
	}
}

void
reactor::stop()
{
	m_stopped = true;
	m_sockpair[1].write_integral(1);
	m_future.wait();
}

void
reactor::add_handler(sock_t s, event e, handler h, bool one_shot)
{
	ev_info ei { e, std::move(h), one_shot };

	std::lock_guard<std::mutex> guard(m_lock);

	ev_handler_type::iterator H;
	H = m_handlers.find(s);
	if (H != m_handlers.end()) {
		bool found = false;
		for (auto &evinfo : H->second) {
			if (evinfo.e == e) {
				evinfo.h = std::move(h);
				evinfo.o = one_shot;
				found = true;
				break;
			}
		}

		if (!found)
			H->second.emplace_back(ei);
	} else {
		ev_info_type eivec(1, ei);
		m_handlers.insert(std::make_pair(s, eivec));
	}

	m_sockpair[1].write_integral(1);
}

void
reactor::remove_handler(sock_t s)
{
	std::lock_guard<std::mutex> guard(m_lock);

	m_handlers.erase(s);

	m_sockpair[1].write_integral(1);
}
void
reactor::remove_handler(sock_t s, event e)
{
	std::lock_guard<std::mutex> guard(m_lock);

	ev_handler_type::iterator H;
	H = m_handlers.find(s);
	if (H != m_handlers.end()) {
		ev_info_type::iterator E;
		for (E = H->second.begin(); E != H->second.end(); ++E) {
			if (E->e == e) {
				H->second.erase(E);
				break;
			}
		}

		if (H->second.empty())
			m_handlers.erase(s);
	}

	m_sockpair[1].write_integral(1);
}


} // namespace poller
} // namespace net
} // namespace snf
