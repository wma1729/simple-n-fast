#include "timeutil.h"
#include "body.h"
#include "headers.h"
#include "json.h"
#include "router.h"

snf::http::response
ok_response(const snf::http::request &rqst)
{
	snf::datetime now(true);

	snf::http::headers hdrs;
	hdrs.date(now);
	hdrs.connection(snf::http::CONNECTION_CLOSE);
	hdrs.content_type(snf::http::CONTENT_TYPE_T_APPLICATION, snf::http::CONTENT_TYPE_ST_JSON);

	snf::json::value json_body = OBJECT {
		KVPAIR("response",
				ARRAY {
					OBJECT {
						KVPAIR("title", snf::http::reason_phrase(snf::http::status_code::OK)),
						KVPAIR("detail", "successful")
					}
				}
		)
	};

	snf::http::body *resp_body = snf::http::body_factory::instance().from_string(json_body.str(true));

	snf::http::response_builder resp_bldr;

	return resp_bldr
		.with_version(1, 1)
		.with_status(snf::http::status_code::OK)
		.with_headers(std::move(hdrs))
		.with_body(resp_body)
		.build();
}

class routertest : public snf::tf::test
{
private:
	static constexpr const char *class_name = "routertest";

public:
	routertest() : snf::tf::test() { std::srand(static_cast<unsigned int>(time(0))); }

	~routertest() {}

	virtual const char *name() const
	{
		return "Router Test";
	}

	virtual const char *description() const
	{
		return "Tests routing";
	}

	bool ok()
	{
		TEST_LOG("status ok");

		snf::http::headers hdrs;
		hdrs.add("Host: h3:80");

		try {
			TEST_LOG("handling /a/b/c");
			snf::http::request_builder reqbldr1;
			snf::http::request rqst1 = std::move(
				reqbldr1.method("GET")
						.with_uri("/a/b/c")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp1 = std::move(snf::http::router::instance().handle(rqst1));

			ASSERT_EQ(snf::http::status_code, resp1.get_status(), snf::http::status_code::OK, "status matches");

			TEST_LOG("handling /a/b/c/d");
			snf::http::request_builder reqbldr2;
			snf::http::request rqst2 = std::move(
				reqbldr2.method("GET")
						.with_uri("/a/b/c/d")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp2 = std::move(snf::http::router::instance().handle(rqst2));

			ASSERT_EQ(snf::http::status_code, resp2.get_status(), snf::http::status_code::OK, "status matches");

			TEST_LOG("handling /p/q/r");
			snf::http::request_builder reqbldr3;
			snf::http::request rqst3 = std::move(
				reqbldr3.method("GET")
						.with_uri("/p/q/r")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp3 = std::move(snf::http::router::instance().handle(rqst3));

			ASSERT_EQ(snf::http::status_code, resp3.get_status(), snf::http::status_code::OK, "status matches");
		} catch (const snf::http::not_found &ex) {
			std::cerr << "not found: " << ex.what() << std::endl;
			return false;
		} catch (const snf::http::not_implemented &ex) {
			std::cerr << "not implemented: " << ex.what() << std::endl;
			return false;
		} catch (const std::invalid_argument &ex) {
			std::cerr << "invalid argument: " << ex.what() << std::endl;
			return false;
		} catch (std::exception &ex) {
			std::cerr << "generic error: " << ex.what() << std::endl;
			return false;
		}

		return true;
	}

	bool not_found()
	{
		TEST_LOG("status not found");

		snf::http::headers hdrs;
		hdrs.add("Host: h2:80");

		try {
			snf::http::request_builder reqbldr1;
			snf::http::request rqst1 = std::move(
				reqbldr1.method("GET")
						.with_uri("/a/b/c/d/e")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp1 = std::move(snf::http::router::instance().handle(rqst1));
			return false;
		} catch (const snf::http::not_found &ex) {
			std::cerr << "not found as expected: " << ex.what() << std::endl;
		}

		try {
			snf::http::request_builder reqbldr2;
			snf::http::request rqst2 = std::move(
				reqbldr2.method("GET")
						.with_uri("/a/f/c/d")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp2 = std::move(snf::http::router::instance().handle(rqst2));
			return false;
		} catch (const snf::http::not_found &ex) {
			std::cerr << "not found as expected: " << ex.what() << std::endl;
		}

		try {
			snf::http::request_builder reqbldr3;
			snf::http::request rqst3 = std::move(
				reqbldr3.method("GET")
						.with_uri("/p/q/r/s")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp3 = std::move(snf::http::router::instance().handle(rqst3));
			return false;
		} catch (const snf::http::not_found &ex) {
			std::cerr << "not found as expected: " << ex.what() << std::endl;
		}

		return true;
	}

	bool not_implemented()
	{
		TEST_LOG("status not implemented");

		snf::http::headers hdrs;
		hdrs.add("Host: h3:80");

		try {

			snf::http::request_builder reqbldr1;
			snf::http::request rqst1 = std::move(
				reqbldr1.method("GET")
						.with_uri("/a/b")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp1 = std::move(snf::http::router::instance().handle(rqst1));
			return false;
		} catch (const snf::http::not_implemented &ex) {
			std::cerr << "not implemented as expected: " << ex.what() << std::endl;
		}

		try {
			snf::http::request_builder reqbldr2;
			snf::http::request rqst2 = std::move(
				reqbldr2.method("GET")
						.with_uri("/a/b/f")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp2 = std::move(snf::http::router::instance().handle(rqst2));
			return false;
		} catch (const snf::http::not_implemented &ex) {
			std::cerr << "not implemented as expected: " << ex.what() << std::endl;
		}

		try {
			snf::http::request_builder reqbldr3;
			snf::http::request rqst3 = std::move(
				reqbldr3.method("GET")
						.with_uri("/p/q")
						.with_version(1, 1)
						.with_headers(hdrs)
						.build());
			snf::http::response resp3 = std::move(snf::http::router::instance().handle(rqst3));
			return false;
		} catch (const snf::http::not_implemented &ex) {
			std::cerr << "not implemented as expected: " << ex.what() << std::endl;
		}

		return true;
	}

	virtual bool execute(const snf::config *conf)
	{
		TEST_LOG("adding a/b/c");
		snf::http::router::instance().add("a/b/c", ok_response);

		TEST_LOG("adding a/b/{var}/d");
		snf::http::router::instance().add("a/b/{var}/d", ok_response);

		TEST_LOG("adding p/{var1}/r");
		snf::http::router::instance().add("p/{var1}/r", ok_response);

		if (!ok())
			return false;

		if (!not_found())
			return false;

		if (!not_implemented())
			return false;

		return true;
	}
};
