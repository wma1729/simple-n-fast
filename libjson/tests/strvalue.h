#include "json.h"

class str_value : public snf::tf::test
{
public:
	str_value() : snf::tf::test() {}
	~str_value() {}

	virtual const char *name() const
	{
		return "StringValue";
	}

	virtual const char *description() const
	{
		return "Tests string values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		const char *str_1 = "test_string_1";
		const std::string str_2("test_string_2");
		std::string str_3("test_string_3");
		snf::json::value sv_1(str_1);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"test_string_1\"", "string content check");
		snf::json::value sv_2(str_2);
		ASSERT_EQ(const std::string &, sv_2.str(false), "\"test_string_2\"", "string content check");
		snf::json::value sv_3(std::move(str_3));
		ASSERT_EQ(const std::string &, sv_3.str(false), "\"test_string_3\"", "string content check");

		str_1 = "test_string_1";
		sv_1 = str_3;
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"test_string_3\"", "string content check");
		sv_2 = str_1;
		ASSERT_EQ(const std::string &, sv_2.str(false), "\"test_string_1\"", "string content check");
		sv_3 = str_2;
		ASSERT_EQ(const std::string &, sv_3.str(false), "\"test_string_2\"", "string content check");

		sv_1 = "\\u81ea\\u7531.txt";
		m_strm << "check utf-8 string (unescaped): " << sv_1.get_string();
		ASSERT_EQ(const std::string &, sv_1.get_string(), "自由.txt", m_strm.str());
		m_strm.str("");
		m_strm << "check utf-8 string (escaped): " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"\\u81ea\\u7531.txt\"", m_strm.str());
		m_strm.str("");

		sv_1 = "a\\u005Cb";
		m_strm << "check utf-8 string (unescaped): " << sv_1.get_string();
		ASSERT_EQ(const std::string &, sv_1.get_string(), "a\\b", m_strm.str());
		m_strm.str("");
		m_strm << "check utf-8 string (escaped): " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"a\\\\b\"", m_strm.str());
		m_strm.str("");

		sv_1 = "\\u00e9es permettant \\u2019acc\\u00e9der \\u00e0 des donn\\u00e9es issues de";
		m_strm << "check utf-8 string (unescaped): " << sv_1.get_string();
		ASSERT_EQ(const std::string &, sv_1.get_string(), "ées permettant ’accéder à des données issues de", m_strm.str());
		m_strm.str("");
		m_strm << "check utf-8 string (escaped): " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"\\u00e9es permettant \\u2019acc\\u00e9der \\u00e0 des donn\\u00e9es issues de\"", m_strm.str());
		m_strm.str("");

		sv_1 = "\u00a3";
		m_strm << "check utf-8 string (unescaped): " << sv_1.get_string();
		ASSERT_EQ(const std::string &, sv_1.get_string(), "£", m_strm.str());
		m_strm.str("");
		m_strm << "check utf-8 string (escaped): " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"\\u00a3\"", m_strm.str());
		m_strm.str("");

		snf::json::object obj {
			std::make_pair("key_1", "value_1"),
			std::make_pair("key_2", "value_2")
		};

		std::string expr_1 =
R"({ "key_1" : "value_1", "key_2" : "value_2" })";

		std::string expr_2 =
R"({
  "key_1" : "value_1",
  "key_2" : "value_2"
})";

		m_strm << "obj: " << std::endl << obj.str(false);
		ASSERT_EQ(const std::string &, obj.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "obj (pretty): " << std::endl << obj.str(true);
		ASSERT_EQ(const std::string &, obj.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::value sv = snf::json::from_string(expr_2);
		m_strm << "json object from string";
		ASSERT_EQ(const std::string &, sv.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::array arr {
			"value_1",
			"value_2"
		};

		expr_1 =
R"([ "value_1", "value_2" ])";

		expr_2 =
R"([
  "value_1",
  "value_2"
])";

		m_strm << "arr: " << std::endl << arr.str(false);
		ASSERT_EQ(const std::string &, arr.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "arr (pretty): " << std::endl << arr.str(true);
		ASSERT_EQ(const std::string &, arr.str(true), expr_2, m_strm.str());
		m_strm.str("");

		sv = snf::json::from_string(expr_1);
		m_strm << "json array from string";
		ASSERT_EQ(const std::string &, sv.str(false), expr_1, m_strm.str());
		m_strm.str("");

		return true;
	}
};
