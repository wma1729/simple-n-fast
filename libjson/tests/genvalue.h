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
		snf::json::value v_1 = OBJECT {
			KVPAIR("key1", "str_1"),
			KVPAIR("key2", true),
			KVPAIR("key3", false),
			KVPAIR("key4", 456),
			KVPAIR("key5", 1.2e+10),
			KVPAIR("key6", nullptr),
			KVPAIR("key7",
				ARRAY {
					"str_2", true, false, 123, 3.14, nullptr
				}
			)
		};

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

		snf::json::value v_3 = ARRAY {
			12345.6,
			true,
			nullptr,
			OBJECT {
				KVPAIR("key1", "val_1"),
				KVPAIR("key2", 6983467)
			}
		};

		expr_1 = R"([ 12345.6, true, null, { "key1" : "val_1", "key2" : 6983467 } ])";
		expr_2 =
R"([
  12345.6,
  true,
  null,
  {
    "key1" : "val_1",
    "key2" : 6983467
  }
])";

		m_strm << "array: " << v_3.str(false);
		ASSERT_EQ(const std::string &, v_3.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array(pretty): " << v_3.str(true);
		ASSERT_EQ(const std::string &, v_3.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::value v_4 = snf::json::from_string(expr_2);

		ASSERT_EQ(const std::string &, v_4.str(false), expr_1, "object matches");
		ASSERT_EQ(const std::string &, v_4.str(true), expr_2, "pretty object matches");

		return true;
	}
};
