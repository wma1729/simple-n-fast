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
			std::ostringstream oss;

			snf::http::request_builder reqbldr1;
			snf::http::request_builder reqbldr2;
			snf::http::request_builder reqbldr3;

			snf::http::request rqst1 = std::move(
				reqbldr1.method("GET")
					.with_uri("/hello.txt")
					.with_version(1, 1)
					.build());
			oss << rqst1;

			std::cout << oss.str() << std::endl;

			snf::http::request rqst2 = std::move(reqbldr2.request_line(oss.str()).build());

			ASSERT_EQ(snf::http::method_type, snf::http::method_type::M_GET, rqst2.get_method(), "method matches");
			ASSERT_EQ(int, 1, rqst2.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 1, rqst2.get_version().m_minor, "minor HTTP version matches");

			snf::http::request rqst3 = std::move(reqbldr3.request_line("POST http://www.example.com/hello.txt HTTP/1.0").build());
			ASSERT_EQ(snf::http::method_type, snf::http::method_type::M_POST, rqst3.get_method(), "method matches");
			ASSERT_EQ(int, 1, rqst3.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 0, rqst3.get_version().m_minor, "minor HTTP version matches");

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
			std::ostringstream oss;

			snf::http::response_builder respbldr1;
			snf::http::response_builder respbldr2;
			snf::http::response_builder respbldr3;

			snf::http::response resp1 = std::move(
				respbldr1.with_version("HTTP/1.1")
					.with_status(snf::http::status_code::OK)
					.build());

			oss << resp1;

			std::cout << oss.str() << std::endl;

			snf::http::response resp2 = std::move(respbldr2.response_line(oss.str()).build());
			ASSERT_EQ(int, 1, resp2.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 1, resp2.get_version().m_minor, "minor HTTP version matches");
			ASSERT_EQ(snf::http::status_code, snf::http::status_code::OK, resp2.get_status(), "HTTP status matches");
			ASSERT_EQ(const std::string &, "OK", resp2.get_reason(), "reason string matches");

			snf::http::response resp3 = std::move(respbldr3.response_line("HTTP/2.0 200 NOT OK").build());
			ASSERT_EQ(int, 2, resp3.get_version().m_major, "major HTTP version matches");
			ASSERT_EQ(int, 0, resp3.get_version().m_minor, "minor HTTP version matches");
			ASSERT_EQ(snf::http::status_code, snf::http::status_code::OK, resp3.get_status(), "HTTP status matches");
			ASSERT_EQ(const std::string &, "NOT OK", resp3.get_reason(), "reason string matches");
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
