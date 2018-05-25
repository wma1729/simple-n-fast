#include "json.h"

class bool_value : public snf::tf::test
{
public:
	bool_value() : snf::tf::test() {}
	~bool_value() {}

	virtual const char *name() const
	{
		return "BooleanValue";
	}

	virtual const char *description() const
	{
		return "Tests boolean values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value tv(true);
		ASSERT_EQ(const std::string &, tv.str(false), "true", "check for true value");

		snf::json::value fv(false);
		ASSERT_EQ(const std::string &, fv.str(false), "false", "check for false value");

		snf::json::object object_1 { std::make_pair("tv", tv) };

		std::string expr_1 = "{ \"tv\" : true }";
		std::string expr_2 = "{\n  \"tv\" : true\n}";

		m_strm << "object_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::object object_2; object_2.add("fv", fv);

		expr_1 = "{ \"fv\" : false }";
		expr_2 = "{\n  \"fv\" : false\n}";

		m_strm << "object_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		fv = snf::json::from_string(expr_2);
		m_strm << "json object from string";
		ASSERT_EQ(const std::string &, fv.str(true), expr_2, m_strm.str());
		m_strm.str("");

		tv = true;
		fv = false;

		snf::json::array array_1 { tv };

		expr_1 = "[ true ]";
		expr_2 = "[\n  true\n]";

		m_strm << "array_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		snf::json::array array_2; array_2.add(fv);

		expr_1 = "[ false ]";
		expr_2 = "[\n  false\n]";

		m_strm << "array_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		fv = snf::json::from_string(expr_1);
		m_strm << "json array from string";
		ASSERT_EQ(const std::string &, fv.str(false), expr_1, m_strm.str());
		m_strm.str("");

		tv = true;
		fv = false;

		m_strm << "boolean value is identifiable";
		ASSERT_EQ(bool, tv.is_boolean(), true, m_strm.str());
		ASSERT_EQ(bool, fv.is_boolean(), true, m_strm.str());
		m_strm.str("");

		m_strm << "value is true";
		ASSERT_EQ(bool, tv.get_boolean(), true, m_strm.str());
		m_strm.str("");

		m_strm << "value is false";
		ASSERT_EQ(bool, fv.get_boolean(), false, m_strm.str());
		m_strm.str("");

		return true;
	}
};
