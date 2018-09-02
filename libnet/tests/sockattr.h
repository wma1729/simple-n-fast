#include "net.h"

class sock_attr : public snf::tf::test
{
private:
	static constexpr const char *class_name = "sock_attr";

public:
	sock_attr() : snf::tf::test() {}
	~sock_attr() {}

	virtual const char *name() const
	{
		return "SocketAttributes";
	}

	virtual const char *description() const
	{
		return "Tests socket attributes";
	}

	virtual bool execute(const snf::config *conf)
	{
		try {
			snf::net::socket s(AF_INET, snf::net::socket_type::tcp);

			// keepalive test
			if (s.keepalive()) {
				std::cout << "keep alive is enabled by default" << std::endl;
				s.keepalive(false);
				ASSERT_EQ(bool, false, s.keepalive(), "keep alive is disabled");
			} else {
				std::cout << "keep alive is disabled by default" << std::endl;
				s.keepalive(true);
				ASSERT_EQ(bool, true, s.keepalive(), "keep alive is enabled");
			}

			// reuseaddr test
			if (s.reuseaddr()) {
				std::cout << "reuse address is enabled by default" << std::endl;
				s.reuseaddr(false);
				ASSERT_EQ(bool, false, s.keepalive(), "reuse address is disabled");
			} else {
				std::cout << "reuse address is disabled by default" << std::endl;
				s.reuseaddr(true);
				ASSERT_EQ(bool, true, s.keepalive(), "reuse address is enabled");
			}

			// linger test
			snf::net::socket::linger_type lt;
			lt = s.linger();
			ASSERT_EQ(snf::net::socket::linger_type, lt, snf::net::socket::linger_type::dflt, "default linger type");

			s.linger(snf::net::socket::linger_type::none);
			lt = s.linger();
			ASSERT_EQ(snf::net::socket::linger_type, lt, snf::net::socket::linger_type::none, "none linger type");

			s.linger(snf::net::socket::linger_type::timed, 30);
			int to = 0;
			lt = s.linger(&to);
			ASSERT_EQ(snf::net::socket::linger_type, lt, snf::net::socket::linger_type::timed, "timed linger type");
			ASSERT_EQ(int, to, 30, "linger delay matches");

			// rcvbuf test
			int rbuflen = s.rcvbuf();
			std::cout << "default receive buffer size is " << rbuflen << std::endl;
			s.rcvbuf(262144);
			rbuflen = s.rcvbuf();
			std::cout << "receive buffer size is " << rbuflen << std::endl;
			ASSERT_GE(int, rbuflen, 262144, "receive buffer size matches");

			// sndbuf test
			int sbuflen = s.sndbuf();
			std::cout << "default send buffer size is " << sbuflen << std::endl;
			s.sndbuf(262144);
			sbuflen = s.sndbuf();
			std::cout << "send buffer size is " << sbuflen << std::endl;
			ASSERT_GE(int, sbuflen, 262144, "send buffer size matches");

#if !defined(_WIN32)
			// rcvtimeo test
			time_t rto = s.rcvtimeout();
			std::cout << "default receive timeout is " << rto << " msec" << std::endl;
			s.rcvtimeout(30000);
			rto = s.rcvtimeout();
			std::cout << "receive timeout is " << rto << " msec" << std::endl;
			ASSERT_EQ(int64_t, rto, 30000, "receive timeout matches");

			// sndtimeo test
			time_t sto = s.sndtimeout();
			std::cout << "default send timeout is " << sto << " msec" << std::endl;
			s.sndtimeout(30000);
			sto = s.sndtimeout();
			std::cout << "send timeout is " << sto << " msec" << std::endl;
			ASSERT_EQ(int64_t, sto, 30000, "send timeout matches");
#endif

			// tcpnodelay test
			if (s.tcpnodelay()) {
				std::cout << "tcp nodelay is enabled by default" << std::endl;
				s.tcpnodelay(false);
				ASSERT_EQ(bool, false, s.keepalive(), "tcp nodelay is disabled");
			} else {
				std::cout << "tcp nodelay is disabled by default" << std::endl;
				s.tcpnodelay(true);
				ASSERT_EQ(bool, true, s.keepalive(), "tcp nodelay is enabled");
			}

			// blocking
			s.blocking(false);
			std::cout << "socket is non-blocking now" << std::endl;
			s.blocking(true);
			std::cout << "socket is blocking now" << std::endl;

		} catch (std::invalid_argument ex) {
			std::cerr << "invalid argument: " << ex.what() << std::endl;
		} catch (std::system_error ex) {
			std::cerr << "system error: " << ex.code() << std::endl;
			std::cerr << ex.what() << std::endl;
		}

		return true;
	}
};
