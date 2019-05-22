#include "net.h"
#include "reactor.h"

namespace snf {
namespace net {

class sockpair_handler : public handler
{
private:
	socket  *m_reader = nullptr;

public:
	sockpair_handler(socket *reader)
		: m_reader(reader)
	{
	}

	virtual ~sockpair_handler()
	{
	}

	virtual bool operator()(sock_t s, event e) override
	{
		if (m_reader) {
			int dummy = 0;
			m_reader->read_integral(&dummy, POLL_WAIT_NONE);
			return true;
		}
		return false;
	}
};

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
reactor::process_poll_vector(std::vector<pollfd> &poll_vec)
{
	std::chrono::system_clock::time_point now = 
		std::chrono::system_clock::now();

	for (auto &fdelem : poll_vec) {
		std::lock_guard<std::mutex> guard(m_lock);

		ev_handler_type::iterator H;
		H = m_handlers.find(fdelem.fd);
		if (H != m_handlers.end()) {
			ev_info_type::iterator E = H->second.begin();
			while (E != H->second.end()) {
				std::unique_ptr<ev_info> &uptr = *E;
				bool ok = true;

				if (fdelem.revents != 0) {
					short e = static_cast<short>(uptr->e);

					if (fdelem.revents & POLLERR) {
						ok = (*uptr->h)(fdelem.fd, event::error);
					} else if (fdelem.revents & POLLHUP) {
						ok = (*uptr->h)(fdelem.fd, event::hup);
					} else if (fdelem.revents & POLLNVAL) {
						ok = (*uptr->h)(fdelem.fd, event::invalid);
					} else if (fdelem.revents & e) {
						ok = (*uptr->h)(fdelem.fd, uptr->e);
						if (ok && (uptr->to != std::chrono::milliseconds::zero()))
							uptr->exp = now + uptr->to;
					}
				} else {
					if (now > uptr->exp)
						ok = (*uptr->h)(fdelem.fd, event::timeout);
				}

				if (!ok) {
					E = H->second.erase(E);			
				} else {
					++E;
				}
			}

			if (H->second.empty()) 
				m_handlers.erase(fdelem.fd);
		}
	}
}

reactor::reactor(int to)
	: m_timeout(to)
	, m_sockpair(std::move(snf::net::socket::socketpair()))
{
	m_sockpair[0].blocking(false);
	sock_t s = m_sockpair[0];

	add_handler(s, event::read, new sockpair_handler(&(m_sockpair[0])));

	m_future = std::async(std::launch::async, &reactor::start, this);
}

void
reactor::start()
{
	int nready;
	int syserr;
	std::vector<pollfd> poll_vec;

	m_stopped = false;

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
			process_poll_vector(poll_vec);
		}
	}
}

void
reactor::stop()
{
	if (!m_stopped) {
		m_stopped = true;
		m_sockpair[1].write_integral(1);
	}
}

void
reactor::add_handler(sock_t s, event e, handler *h, int to)
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
				ei->h.reset(h);
				found = true;
				break;
			}
		}

		if (!found) {
			std::unique_ptr<ev_info> ei(new ev_info(e, to, h));
			H->second.emplace_back(std::move(ei));
		}
	} else {
		ev_info_type eivec;
		std::unique_ptr<ev_info> ei(new ev_info(e, to, h));
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


} // namespace net
} // namespace snf
