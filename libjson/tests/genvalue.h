#include "json.h"

class gen_value : public snf::tf::test
{
public:
	gen_value() : snf::tf::test() {}
	~gen_value() {}

	virtual const char *name() const
	{
		return "GenericValue";
	}

	virtual const char *description() const
	{
		return "Tests all values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value v_1 ({
			std::make_pair("key1", "str_1"),
			std::make_pair("key2", true),
			std::make_pair("key3", false),
			std::make_pair("key4", 456),
			std::make_pair("key5", 1.2e+10),
			std::make_pair("key6", snf::json::value {} ),
			std::make_pair("key7",
				snf::json::array {
					"str_2", true, false, 123, 3.14, snf::json::value {}
				}
			)
		});

		std::string expr_1 =
R"({ "key1" : "str_1", "key2" : true, "key3" : false, "key4" : 456, "key5" : 1.2e+10, "key6" : null, "key7" : [ "str_2", true, false, 123, 3.14, null ] })";

		std::string expr_2 =
R"({
  "key1" : "str_1",
  "key2" : true,
  "key3" : false,
  "key4" : 456,
  "key5" : 1.2e+10,
  "key6" : null,
  "key7" : [
    "str_2",
    true,
    false,
    123,
    3.14,
    null
  ]
})";

		m_strm << "object: " << v_1.str(false);
		ASSERT_EQ(const std::string &, v_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object(pretty): " << v_1.str(true);
		ASSERT_EQ(const std::string &, v_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::value v_2 = snf::json::from_string(expr_2);

		ASSERT_EQ(const std::string &, v_2.str(false), expr_1, "object matches");
		ASSERT_EQ(const std::string &, v_2.str(true), expr_2, "pretty object matches");

		return true;
	}
};
