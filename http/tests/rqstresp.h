#include "headers.h"
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
		TEST_LOG("HTTP Request Line Test");

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

			TEST_LOG(oss.str());

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
		TEST_LOG("HTTP Response Line Test");

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

			TEST_LOG(oss.str());

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
		TEST_LOG("Headers Test");

		try {
			TEST_LOG("Valid content length");

			snf::http::headers hdrs1;
			hdrs1.add("content-Length: 30\r\n");
			ASSERT_EQ(size_t, 30, hdrs1.content_length(), "content length matches");
			ASSERT_EQ(bool, false, hdrs1.is_message_chunked(), "message not chunked");

			snf::http::headers hdrs2;
			std::ostringstream oss;
			oss << "Content-length: " << ULLONG_MAX;
			hdrs2.add(oss.str());
			ASSERT_EQ(size_t, ULLONG_MAX , hdrs2.content_length(), "content length matches");
			ASSERT_EQ(bool, false, hdrs2.is_message_chunked(), "message not chunked");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		} catch (const std::out_of_range &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Invalid content length");

			snf::http::headers hdrs3;
			hdrs3.add("coNTEnt-Length: dummy-string");
			TEST_FAIL("No exception thrown");

		} catch (const snf::http::bad_message &ex) {
			TEST_PASS(ex.what());
		}

		try {
			TEST_LOG("Negative content length");

			snf::http::headers hdrs4;
			hdrs4.add("content-length: -256\r\n");
			TEST_FAIL("No exception thrown");

		} catch (const snf::http::bad_message &ex) {
			TEST_PASS(ex.what());
		}

		try {
			TEST_LOG("Supported transfer encoding");

			snf::http::headers hdrs5;
			hdrs5.add("transfer-encoding:     chunked\r\n\r\n");
			const std::vector<snf::http::token> &xfer_codings = hdrs5.transfer_encoding();
			ASSERT_EQ(const std::string &, "chunked", xfer_codings[0].m_name, "transfer encoding matches");
			ASSERT_EQ(bool, true, hdrs5.is_message_chunked(), "message is chunked");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Unsupported transfer encoding");

			snf::http::headers hdrs6;
			hdrs6.add("Transfer-encoding: gzip, compress\r\n");
			const std::vector<snf::http::token> &xfer_codings = hdrs6.transfer_encoding();
			ASSERT_EQ(size_t, 2, xfer_codings.size(), "coding count matches");
			ASSERT_EQ(const std::string &, "gzip", xfer_codings[0].m_name, "transfer encoding matches");
			ASSERT_EQ(const std::string &, "compress", xfer_codings[1].m_name, "transfer encoding matches");

		} catch (const snf::http::not_implemented &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Host names");

			snf::http::headers hdrs7;
			in_port_t port = 0;

			hdrs7.add("host: www.example.com \r\n");
			ASSERT_EQ(const std::string &, "www.example.com", hdrs7.host(&port), "host matches");
			ASSERT_EQ(in_port_t, 0, port, "port matches");

			snf::http::headers hdrs8;
			hdrs8.add("host: \"bharat:8080\"  	\r\n");
			ASSERT_EQ(const std::string &, "bharat", hdrs8.host(&port), "host matches");
			ASSERT_EQ(in_port_t, 8080, port, "port matches");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Valid connection");

			snf::http::headers hdrs8;
			hdrs8.add("Connection: close");
			const std::vector<std::string> &conval1 = hdrs8.connection();
			ASSERT_EQ(const std::string &, "close", conval1[0], "connection matches");

			snf::http::headers hdrs9;
			hdrs9.add("Connection: keep-alive");
			const std::vector<std::string> &conval2 = hdrs9.connection();
			ASSERT_EQ(const std::string &, "keep-alive", conval2[0], "connection matches");

			snf::http::headers hdrs10;
			hdrs10.add("Connection: upgrade");
			const std::vector<std::string> &conval3 = hdrs10.connection();
			ASSERT_EQ(const std::string &, "upgrade", conval3[0], "connection matches");
		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Invalid connection");

			snf::http::headers hdrs11;
			hdrs11.add("Connection: weird");
			TEST_FAIL("No exception thrown");
		} catch (const snf::http::not_implemented &ex) {
			TEST_PASS(ex.what());
		} 

		try {
			TEST_LOG("Content Type");

			snf::http::headers hdrs12;
			hdrs12.add("content-type: text/plain;charset=utf-8\r\n");
			const snf::http::media_type &mt1 = hdrs12.content_type();
			ASSERT_EQ(const std::string &, "text", mt1.m_type, "type matches");
			ASSERT_EQ(const std::string &, "plain", mt1.m_subtype, "subtype matches");
			ASSERT_EQ(const std::string &, "charset", mt1.m_parameters[0].first, "param name matches");
			ASSERT_EQ(const std::string &, "utf-8", mt1.m_parameters[0].second, "param value matches");

			snf::http::headers hdrs13;
			hdrs13.add("content-type: text/plain;charset=UTF-8\r\n");
			const snf::http::media_type &mt2 = hdrs13.content_type();
			ASSERT_EQ(const std::string &, "text", mt2.m_type, "type matches");
			ASSERT_EQ(const std::string &, "plain", mt2.m_subtype, "subtype matches");
			ASSERT_EQ(const std::string &, "charset", mt2.m_parameters[0].first, "param name matches");
			ASSERT_EQ(const std::string &, "UTF-8", mt2.m_parameters[0].second, "param value matches");

			snf::http::headers hdrs14;
			hdrs14.add("content-type: Text/PLAIN;charset=\"utf-8\"\r\n");
			const snf::http::media_type &mt3 = hdrs14.content_type();
			ASSERT_EQ(const std::string &, "text", mt3.m_type, "type matches");
			ASSERT_EQ(const std::string &, "plain", mt3.m_subtype, "subtype matches");
			ASSERT_EQ(const std::string &, "charset", mt3.m_parameters[0].first, "param name matches");
			ASSERT_EQ(const std::string &, "utf-8", mt3.m_parameters[0].second, "param value matches");

			snf::http::headers hdrs15;
			hdrs15.add("content-type: text/plain; charset=\"UTF-8\"\r\n");
			const snf::http::media_type &mt4 = hdrs15.content_type();
			ASSERT_EQ(const std::string &, "text", mt4.m_type, "type matches");
			ASSERT_EQ(const std::string &, "plain", mt4.m_subtype, "subtype matches");
			ASSERT_EQ(const std::string &, "charset", mt4.m_parameters[0].first, "param name matches");
			ASSERT_EQ(const std::string &, "UTF-8", mt4.m_parameters[0].second, "param value matches");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("TE");

			snf::http::headers hdrs16;
			hdrs16.te("trailers, deflate;q=0.5");
			hdrs16.add("TE: ");
			const std::vector<snf::http::token> &codings = hdrs16.te();
			ASSERT_EQ(size_t, 2, codings.size(), "TE coding size matches");
			ASSERT_EQ(const std::string &, "trailers", codings[0].m_name, "TE coding matches");
			ASSERT_EQ(const std::string &, "deflate", codings[1].m_name, "TE coding matches");
			ASSERT_EQ(const std::string &, "q", codings[1].m_parameters[0].first, "TE coding matches");
			ASSERT_EQ(const std::string &, "0.5", codings[1].m_parameters[0].second, "TE coding matches");
			ASSERT_EQ(bool, true, hdrs16.has_trailers(), "message has trailers");

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
		}

		try {
			TEST_LOG("Via");

			snf::http::headers hdrs17;
			hdrs17.intermediary("HTTP/1.1 www.example.com:8080");
			hdrs17.add("via: 1.0 www.simplenfast.org (a comment), 1.1 pseudonym");
			const std::vector<snf::http::via> &intermediaries = hdrs17.intermediary();
			ASSERT_EQ(size_t, 3, intermediaries.size(), "Via size matches");
			std::cout << hdrs17 << std::endl;

		} catch (const snf::http::bad_message &ex) {
			std::cerr << ex.what() << std::endl;
			return false;
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
