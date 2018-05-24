#include "json.h"

class null_value : public snf::tf::test
{
public:
	null_value() : snf::tf::test() {}
	~null_value() {}

	virtual const char *name() const
	{
		return "NullValue";
	}

	virtual const char *description() const
	{
		return "Tests null values in objects and array";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value nv_1;
		ASSERT_EQ(const std::string &, nv_1.str(false), "null", "check for null value");

		snf::json::value nv_2;

		snf::json::object object_1 { std::make_pair("nv", nv_1) };
		snf::json::object object_2; object_2.add("nv", nv_2);

		std::string expr_1 = "{ \"nv\" : null }";
		std::string expr_2 = "{\n  \"nv\" : null\n}";

		m_strm << "object_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		m_strm << "object_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, object_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "object_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, object_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		nv_1 = snf::json::from_string(expr_2);
		m_strm << "json object from string";
		ASSERT_EQ(const std::string &, nv_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		nv_1 = nullptr;
		snf::json::array array_1 { nv_1 };
		snf::json::array array_2; array_2.add(nv_2);

		expr_1 = "[ null ]";
		expr_2 = "[\n  null\n]";

		std::cerr << array_1.str(false) << std::endl;
		std::cerr << array_1.str(true) << std::endl;

		m_strm << "array_1:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_1.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_1 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		m_strm << "array_2:" << std::endl << expr_1;
		ASSERT_EQ(const std::string &, array_2.str(false), expr_1, m_strm.str());
		m_strm.str("");

		m_strm << "array_2 (pretty):" << std::endl << expr_2;
		ASSERT_EQ(const std::string &, array_2.str(true), expr_2, m_strm.str());
		m_strm.str("");

		nv_1 = snf::json::from_string(expr_2);
		m_strm << "json array from string";
		ASSERT_EQ(const std::string &, nv_1.str(true), expr_2, m_strm.str());
		m_strm.str("");

		nv_1 = nullptr;
		m_strm << "null value is identifiable";
		ASSERT_EQ(bool, nv_1.is_null(), true, m_strm.str());
		m_strm.str("");

		return true;
	}
};
