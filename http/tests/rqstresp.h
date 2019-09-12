#include "request.h"
#include "response.h"
#include "status.h"
#include <climits>

class rqstresp : public snf::tf::test
{
private:
	static constexpr const char *class_name = "restresp";

	bool test_rqst()
	{
		std::cout << "HTTP Request Line Test" << std::endl;

		try {
			snf::http::request_builder reqbldr;
			snf::http::request rqst1 = std::move(reqbldr.request_line("GET /hello.txt HTTP/1.1\r\n").build());
			ASSERT_EQ(snf::http::method_type, snf::http::method_type::M_GET, rqst1.get_method(), "method matches");
			ASSERT_EQ(int, 1, rqst1.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 1, rqst1.get_version().m_minor, "minor HTTP version matches");

			snf::http::request rqst2 = std::move(reqbldr.request_line("POST http://www.example.com/hello.txt HTTP/1.0").build());
			ASSERT_EQ(snf::http::method_type, snf::http::method_type::M_POST, rqst2.get_method(), "method matches");
			ASSERT_EQ(int, 1, rqst2.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 0, rqst2.get_version().m_minor, "minor HTTP version matches");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << "bad request: " << ex.what() << std::endl;
			return false;
		}

		return true;
	}

	bool test_resp()
	{
		std::cout << "HTTP Response Line Test" << std::endl;

		try {
			snf::http::response_builder respbldr;
			snf::http::response resp1 = std::move(respbldr.response_line("HTTP/1.1 200 OK\r\n").build());
			ASSERT_EQ(int, 1, resp1.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 1, resp1.get_version().m_minor, "minor HTTP version matches");
			ASSERT_EQ(snf::http::status_code, snf::http::status_code::OK, resp1.get_status(), "HTTP status matches");
			ASSERT_EQ(const std::string &, "OK", resp1.get_reason(), "reason string matches");

			snf::http::response resp2 = std::move(respbldr.response_line("HTTP/2.0 200 NOT OK").build());
			ASSERT_EQ(int, 2, resp2.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 0, resp2.get_version().m_minor, "minor HTTP version matches");
			ASSERT_EQ(snf::http::status_code, snf::http::status_code::OK, resp2.get_status(), "HTTP status matches");
			ASSERT_EQ(const std::string &, "NOT OK", resp2.get_reason(), "reason string matches");
		} catch (const snf::http::bad_message &ex) {
			std::cerr << "bad response: " << ex.what() << std::endl;
			return false;
		}

		return true;
	}

	bool test_hdrs()
	{
		std::cout << "Headers Test" << std::endl;

		try {
			snf::http::headers hdrs1;
			hdrs1.add("Content-Length: 30\r\n");
			ASSERT_EQ(size_t, 30, hdrs1.content_length(), "content length matches");

			snf::http::headers hdrs2;
			std::ostringstream oss;
			oss << "Content-Length: " << ULLONG_MAX;
			hdrs2.add(oss.str());
			ASSERT_EQ(size_t, ULLONG_MAX , hdrs2.content_length(), "content length matches");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		} catch (const std::out_of_range &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			snf::http::headers hdrs3;
			hdrs3.add("Content-Length: dummy-string");
			TEST_FAIL("No exception thrown");
		} catch (const snf::http::bad_message &ex) {
			TEST_PASS(ex.what());
			return true;
		}

		return true;
	}

public:
	rqstresp() : snf::tf::test() {}

	~rqstresp() {}

	virtual const char *name() const
	{
		return "Request/Response";
	}

	virtual const char *description() const
	{
		return "Tests HTTP Request/Response Line";
	}

	virtual bool execute(const snf::config *conf)
	{
		if (!test_rqst())
			return false;

		if (!test_resp())
			return false;

		if (!test_hdrs())
			return false;

		return true;
	}
};
