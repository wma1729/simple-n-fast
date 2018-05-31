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

		sv_1 = "\u81ea\u7531.txt";
		m_strm << "check utf-8 string " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"自由.txt\"", m_strm.str());
		m_strm.str("");

		sv_1 = "a\u005Cb";
		m_strm << "check utf-8 string " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"a\\b\"", m_strm.str());
		m_strm.str("");

		sv_1 = "\u00e9es permettant \u2019acc\u00e9der \u00e0 des donn\u00e9es issues de";
		m_strm << "check utf-8 string " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"ées permettant ’accéder à des données issues de\"", m_strm.str());
		m_strm.str("");

		sv_1 = "\u00a3";
		m_strm << "check utf-8 string " << sv_1.str(false);
		ASSERT_EQ(const std::string &, sv_1.str(false), "\"£\"", m_strm.str());
		m_strm.str("");
		return true;
	}
};
