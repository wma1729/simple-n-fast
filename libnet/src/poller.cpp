#include "net.h"
#include "poller.h"

namespace snf {
namespace net {
namespace poller {

void
reactor::set_poll_vector(std::vector<pollfd> &poll_vec)
{
	poll_vec.clear();

	std::lock_guard<std::mutex> guard(m_lock);

	for (auto &h : m_handlers) {
		pollfd fdelem = { h.first, 0, 0 };
		for (auto &ei : h.second) {
			switch (ei->e) {
				case event::read:
				case event::write:
					fdelem.events |= static_cast<short>(ei->e);
					break;

				default:
					break;
			}
		}

		if (fdelem.events != 0)
			poll_vec.push_back(fdelem);
	}
}

void
reactor::process_poll_vector(std::vector<pollfd> &poll_vec, int nready)
{
	for (auto &fdelem : poll_vec) {
		if (nready && (fdelem.revents != 0)) {

			std::lock_guard<std::mutex> guard(m_lock);

			ev_handler_type::iterator H;
			H = m_handlers.find(fdelem.fd);
			if (H != m_handlers.end()) {

				ev_info_type::iterator E = H->second.begin();
				while (E != H->second.end()) {
					std::unique_ptr<ev_info> &uptr = *E;
					short e = static_cast<short>(uptr->e);
					bool ok = true;

					if (fdelem.revents & POLLERR)
						ok = uptr->h(fdelem.fd, event::error);
					else if (fdelem.revents & POLLHUP)
						ok = uptr->h(fdelem.fd, event::hup);
					else if (fdelem.revents & POLLNVAL)
						ok = uptr->h(fdelem.fd, event::invalid);
					else if (fdelem.revents & e)
						ok = uptr->h(fdelem.fd, uptr->e);

					nready--;

					if (!ok)
						E = H->second.erase(E);			
					else
						++E;
				}

				if (H->second.empty()) 
					m_handlers.erase(fdelem.fd);
			}
		}
	}
}

void
reactor::start()
{
	int nready;
	int syserr;
	std::vector<pollfd> poll_vec;

	while (!m_stopped) {

		syserr = 0;

		set_poll_vector(poll_vec);

		nready = snf::net::poll(poll_vec, m_timeout, &syserr);
		if (SOCKET_ERROR == nready) {
			throw std::system_error(
				syserr,
				std::system_category(),
				"poll failed");
		} else if (nready > 0) {
			process_poll_vector(poll_vec, nready);
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

reactor::reactor(int to)
	: m_timeout(to)
	, m_sockpair(std::move(snf::net::socket::socketpair()))
{
	m_sockpair[0].blocking(false);
	sock_t s = m_sockpair[0];

	add_handler(s, event::read, [this] (sock_t s, event e) {
		int dummy = 0;
		this->m_sockpair[0].read_integral(&dummy, POLL_WAIT_NONE);
		return true;
	});

	m_future = std::async(std::launch::async, &reactor::start, this);
}

void
reactor::add_handler(sock_t s, event e, std::function<bool(sock_t, event)> h)
{
	if (s == INVALID_SOCKET)
		throw std::invalid_argument("invalid socket");

	if ((e != event::read) && (e != event::write))
		throw std::invalid_argument("invalid event type: only read/write could be registered");

	std::lock_guard<std::mutex> guard(m_lock);

	ev_handler_type::iterator H;
	H = m_handlers.find(s);
	if (H != m_handlers.end()) {
		bool found = false;
		for (auto &ei : H->second) {
			if (ei->e == e) {
				ei->h = std::move(h);
				found = true;
				break;
			}
		}

		if (!found) {
			std::unique_ptr<ev_info> ei(new ev_info(e, std::move(h)));
			H->second.emplace_back(std::move(ei));
		}
	} else {
		ev_info_type eivec;
		std::unique_ptr<ev_info> ei(new ev_info(e, std::move(h)));
		eivec.emplace_back(std::move(ei));
		m_handlers[s] = std::move(eivec);
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
		ev_info_type::iterator E = H->second.begin();
		while (E != H->second.end()) {
			if ((*E)->e == e) {
				H->second.erase(E);
				break;
			}
			++E;
		}

		if (H->second.empty())
			m_handlers.erase(s);
	}

	m_sockpair[1].write_integral(1);
}


} // namespace poller
} // namespace net
} // namespace snf
