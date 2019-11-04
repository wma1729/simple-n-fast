#include "json.h"
#include <climits>

class int_value : public snf::tf::test
{
public:
	int_value() : snf::tf::test() {}
	~int_value() {}

	virtual const char *name() const
	{
		return "IntegerValue";
	}

	virtual const char *description() const
	{
		return "Tests integer values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value iv_1(SCHAR_MAX);
		ASSERT_EQ(const std::string &, iv_1.str(false), "127", "check for integer value");
		snf::json::value iv_2(SCHAR_MIN);
		ASSERT_EQ(const std::string &, iv_2.str(false), "-128", "check for integer value");
		snf::json::value iv_3(SHRT_MAX);
		ASSERT_EQ(const std::string &, iv_3.str(false), "32767", "check for integer value");
		snf::json::value iv_4(SHRT_MIN);
		ASSERT_EQ(const std::string &, iv_4.str(false), "-32768", "check for integer value");
		snf::json::value iv_5(INT_MAX);
		ASSERT_EQ(const std::string &, iv_5.str(false), "2147483647", "check for integer value");
		snf::json::value iv_6(INT_MIN);
		ASSERT_EQ(const std::string &, iv_6.str(false), "-2147483648", "check for integer value");
		snf::json::value iv_7(LLONG_MAX);
		ASSERT_EQ(const std::string &, iv_7.str(false), "9223372036854775807", "check for integer value");
		snf::json::value iv_8(LLONG_MIN);
		ASSERT_EQ(const std::string &, iv_8.str(false), "-9223372036854775808", "check for integer value");

		iv_8 = SCHAR_MAX;
		ASSERT_EQ(const std::string &, iv_8.str(false), "127", "check for integer value assignment");
		iv_7 = SCHAR_MIN;
		ASSERT_EQ(const std::string &, iv_7.str(false), "-128", "check for integer value assignment");
		iv_6 = SHRT_MAX;
		ASSERT_EQ(const std::string &, iv_6.str(false), "32767", "check for integer value assignment");
		iv_5 = SHRT_MIN;
		ASSERT_EQ(const std::string &, iv_5.str(false), "-32768", "check for integer value assignment");
		iv_4 = INT_MAX;
		ASSERT_EQ(const std::string &, iv_4.str(false), "2147483647", "check for integer value assignment");
		iv_3 = INT_MIN;
		ASSERT_EQ(const std::string &, iv_3.str(false), "-2147483648", "check for integer value assignment");
		iv_2 = LLONG_MAX;
		ASSERT_EQ(const std::string &, iv_2.str(false), "9223372036854775807", "check for integer value assignment");
		iv_1 = LLONG_MIN;
		ASSERT_EQ(const std::string &, iv_1.str(false), "-9223372036854775808", "check for integer value assignment");

		snf::json::object object_1 { std::make_pair("iv", iv_4) };
		snf::json::object object_2; object_2.add("iv", iv_5);

		std::string expr_1 = "{ \"iv\" : 2147483647 }";
		std::string expr_2 = "{\n  \"iv\" : 2147483647\n}";

		m_strm << "object_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		expr_1 = "{ \"iv\" : -32768 }";
		expr_2 = "{\n  \"iv\" : -32768\n}";

		m_strm << "object_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::value obj = snf::json::from_string(expr_2);
		m_strm << "json object from string";
		ASSERT_EQ(const std::string &, obj.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::array array_1 { iv_2 };
		snf::json::array array_2; array_2.add(iv_7);

		expr_1 = "[ 9223372036854775807 ]";
		expr_2 = "[\n  9223372036854775807\n]";

		m_strm << "array_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		expr_1 = "[ -128 ]";
		expr_2 = "[\n  -128\n]";

		m_strm << "array_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::value arr = snf::json::from_string(expr_1);
		m_strm << "json array from string";
		ASSERT_EQ(const std::string &, arr.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "integer value is identifiable";
		ASSERT_EQ(bool, iv_1.is_integer(), true, m_strm.str());
		m_strm.str("");

		ASSERT_EQ(int64_t,  iv_8.get_integer(), SCHAR_MAX, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_7.get_integer(), SCHAR_MIN, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_6.get_integer(), SHRT_MAX, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_5.get_integer(), SHRT_MIN, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_4.get_integer(), INT_MAX, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_3.get_integer(), INT_MIN, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_2.get_integer(), LLONG_MAX, "integer value preserved");
		ASSERT_EQ(int64_t,  iv_1.get_integer(), LLONG_MIN, "integer value preserved");

		return true;
	}
};
