#include "json.h"
#include <climits>

class real_value : public snf::tf::test
{
public:
	real_value() : snf::tf::test() {}
	~real_value() {}

	virtual const char *name() const
	{
		return "RealValue";
	}

	virtual const char *description() const
	{
		return "Tests real values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value rv_1(.3333);
		ASSERT_EQ(const std::string &, rv_1.str(false), "0.3333", "check for real value");
		snf::json::value rv_2(-.3333);
		ASSERT_EQ(const std::string &, rv_2.str(false), "-0.3333", "check for real value");
		snf::json::value rv_3(0.999);
		ASSERT_EQ(const std::string &, rv_3.str(false), "0.999", "check for real value");
		snf::json::value rv_4(-0.999);
		ASSERT_EQ(const std::string &, rv_4.str(false), "-0.999", "check for real value");
		snf::json::value rv_5(124.999);
		ASSERT_EQ(const std::string &, rv_5.str(false), "124.999", "check for real value");
		snf::json::value rv_6(-124.999);
		ASSERT_EQ(const std::string &, rv_6.str(false), "-124.999", "check for real value");
		snf::json::value rv_7(0.1E+001);
		ASSERT_EQ(const std::string &, rv_7.str(false), "1", "check for real value");
		ASSERT_EQ(bool, rv_7.is_real(), true, "value is real");
		snf::json::value rv_8(-0.1E+001);
		ASSERT_EQ(const std::string &, rv_8.str(false), "-1", "check for real value");
		ASSERT_EQ(bool, rv_8.is_real(), true, "value is real");
		snf::json::value rv_9(0.1E-000001);
		ASSERT_EQ(const std::string &, rv_9.str(false), "0.01", "check for real value");
		snf::json::value rv_10(0.1E-1);
		ASSERT_EQ(const std::string &, rv_10.str(false), "0.01", "check for real value");
		snf::json::value rv_11(-0.1E-1);
		ASSERT_EQ(const std::string &, rv_11.str(false), "-0.01", "check for real value");
		snf::json::value rv_12(123E-3);
		ASSERT_EQ(const std::string &, rv_12.str(false), "0.123", "check for real value");
		snf::json::value rv_13(123E3);
		ASSERT_EQ(const std::string &, rv_13.str(false), "123000", "check for real value");

		std::string expr_1 =
R"({
  "rv_1" : 0.3333,
  "rv_10" : 0.1E-1,
  "rv_11" : -0.1E-1,
  "rv_12" : 123E-3,
  "rv_13" : 123E3,
  "rv_2" : -0.3333,
  "rv_3" : 0.999,
  "rv_4" : -0.999,
  "rv_5" : 124.999,
  "rv_6" : -124.999,
  "rv_7" : 0.1E+001,
  "rv_8" : -0.1E+001,
  "rv_9" : 0.1E-000001
})";

		std::string expr_2 =
R"({
  "rv_1" : 0.3333,
  "rv_10" : 0.01,
  "rv_11" : -0.01,
  "rv_12" : 0.123,
  "rv_13" : 123000,
  "rv_2" : -0.3333,
  "rv_3" : 0.999,
  "rv_4" : -0.999,
  "rv_5" : 124.999,
  "rv_6" : -124.999,
  "rv_7" : 1,
  "rv_8" : -1,
  "rv_9" : 0.01
})";

		snf::json::value obj;

		try {
			obj = snf::json::from_string(expr_1);
		} catch (snf::json::parsing_error ex) {
			std::cerr << ex.what() << " at " << ex.row() << "." << ex.col() << std::endl;
			return false;
		}

		m_strm << "obj:" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, obj.str(true), expr_2, m_strm.str());
		m_strm.str("");

		expr_1 =
R"([
  0.3333,
  0.1E-1,
  -0.1E-1,
  123E-3,
  123E3,
  -0.3333,
  0.999,
  -0.999,
  124.999,
  -124.999,
  0.1E+001,
  -0.1E+001,
  0.1E-000001
])";

		expr_2 =
R"([
  0.3333,
  0.01,
  -0.01,
  0.123,
  123000,
  -0.3333,
  0.999,
  -0.999,
  124.999,
  -124.999,
  1,
  -1,
  0.01
])";

		snf::json::value arr;

		try {
			arr = snf::json::from_string(expr_1);
		} catch (snf::json::parsing_error ex) {
			std::cerr << ex.what() << " at " << ex.row() << "." << ex.col() << std::endl;
			return false;
		}

		m_strm << "arr:" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, arr.str(true), expr_2, m_strm.str());
		m_strm.str("");

		return true;
	}
};
