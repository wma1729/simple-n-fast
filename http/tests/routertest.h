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

	virtual bool execute(const snf::config *conf)
	{
		snf::http::router::instance().add("resources/sub-resource/abc", ok_response);
		snf::http::router::instance().add("resources/sub-resource/{var}/xyz", ok_response);

		snf::http::request_builder reqbldr1;
		snf::http::request rqst1 = std::move(
			reqbldr1.method("GET")
					.with_uri("/resources/sub-resource/sub-sub-resource/xyz")
					.with_version(1, 1)
					.build());
		snf::http::response resp1 = std::move(snf::http::router::instance().handle(rqst1));

		std::ostringstream oss;
		oss << resp1;

		std::cout << oss.str();
		return true;
	}
};
