#include "uri.h"

class uritest : public snf::tf::test
{
private:
	static constexpr const char *class_name = "uritest";

	struct entry {
		std::string     relative;
		std::string     target;
	};

	std::vector<entry> table;

public:
	uritest() : snf::tf::test()
	{
		table.emplace_back( entry { "g:h", "g:h" } );
		table.emplace_back( entry { "g", "http://a/b/c/g" } );
		table.emplace_back( entry { "./g", "http://a/b/c/g" } );
		table.emplace_back( entry { "g/", "http://a/b/c/g/" } );
		table.emplace_back( entry { "/g", "http://a/g" } );
		table.emplace_back( entry { "//g", "http://g" } );
		table.emplace_back( entry { "?y", "http://a/b/c/d;p?y" } );
		table.emplace_back( entry { "g?y", "http://a/b/c/g?y" } );
		table.emplace_back( entry { "#s", "http://a/b/c/d;p?q#s" } );
		table.emplace_back( entry { "g#s", "http://a/b/c/g#s" } );
		table.emplace_back( entry { "g?y#s", "http://a/b/c/g?y#s" } );
		table.emplace_back( entry { ";x", "http://a/b/c/;x" } );
		table.emplace_back( entry { "g;x", "http://a/b/c/g;x" } );
		table.emplace_back( entry { "g;x?y#s", "http://a/b/c/g;x?y#s" } );
		table.emplace_back( entry { "", "http://a/b/c/d;p?q" } );
		table.emplace_back( entry { ".", "http://a/b/c" } ); // spec: http://a/b/c/
		table.emplace_back( entry { "./", "http://a/b/c/" } );
		table.emplace_back( entry { "..", "http://a/b" } ); // spec: http://a/b/
		table.emplace_back( entry { "../", "http://a/b/" } );
		table.emplace_back( entry { "../g", "http://a/b/g" } );
		table.emplace_back( entry { "../..", "http://a" } ); // spec: http://a/
		table.emplace_back( entry { "../../", "http://a" } ); // spec: http://a/
		table.emplace_back( entry { "../../g", "http://a/g" } );
		table.emplace_back( entry { "../../../g", "http://a/g" } );
		table.emplace_back( entry { "../../../../g", "http://a/g" } );
		table.emplace_back( entry { "/./g", "http://a/g" } );
		table.emplace_back( entry { "/../g", "http://a/g" } );
		table.emplace_back( entry { "g.", "http://a/b/c/g." } );
		table.emplace_back( entry { ".g", "http://a/b/c/.g" } );
		table.emplace_back( entry { "g..", "http://a/b/c/g.." } );
		table.emplace_back( entry { "..g", "http://a/b/c/..g" } );
		table.emplace_back( entry { "./../g", "http://a/b/g" } );
		table.emplace_back( entry { "./g/.", "http://a/b/c/g" } ); // spec: http://a/b/c/g/
		table.emplace_back( entry { "g/./h", "http://a/b/c/g/h" } );
		table.emplace_back( entry { "g/../h", "http://a/b/c/h" } );
		table.emplace_back( entry { "g;x=1/./y", "http://a/b/c/g;x=1/y" } );
		table.emplace_back( entry { "g;x=1/../y", "http://a/b/c/y" } );
		table.emplace_back( entry { "g?y/./x", "http://a/b/c/g?y/./x" } );
		table.emplace_back( entry { "g?y/../x", "http://a/b/c/g?y/../x" } );
		table.emplace_back( entry { "g#s/./x", "http://a/b/c/g#s/./x" } );
		table.emplace_back( entry { "g#s/../x", "http://a/b/c/g#s/../x" } );
	}

	~uritest() {}

	virtual const char *name() const
	{
		return "URI";
	}

	virtual const char *description() const
	{
		return "Tests URI";
	}

	virtual void test_merge(const std::string &base)
	{
		snf::http::uri base_uri { base };

		for (auto t : table) {
			std::cout
				<< "base = " << base
				<< ", relative = " << t.relative
				<< ", target = " << t.target
				<< std::endl;

			snf::http::uri relative { t.relative };
			snf::http::uri target = std::move(base_uri.merge(relative));
			std::ostringstream oss;
			oss << target;
			ASSERT_EQ(const std::string &, t.target, oss.str(), "target matches");
		}
	}

	virtual bool execute(const snf::config *conf)
	{
		try {
			std::ostringstream oss;

			snf::http::uri uri_1 { "foo://rd@example.com:8042/over/there?name=ferret#nose" };

			oss.str("");
			oss << uri_1.get_scheme();
			ASSERT_EQ(const std::string &, "foo", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_1.get_userinfo();
			ASSERT_EQ(const std::string &, "rd", oss.str(), "userinfo matches");

			oss.str("");
			oss << uri_1.get_host();
			ASSERT_EQ(const std::string &, "example.com", oss.str(), "host matches");

			oss.str("");
			oss << uri_1.get_port();
			ASSERT_EQ(const std::string &, "8042", oss.str(), "port matches");

			oss.str("");
			oss << uri_1.get_path();
			ASSERT_EQ(const std::string &, "/over/there", oss.str(), "path matches");

			oss.str("");
			oss << uri_1.get_query();
			ASSERT_EQ(const std::string &, "name=ferret", oss.str(), "query matches");

			oss.str("");
			oss << uri_1.get_fragment();
			ASSERT_EQ(const std::string &, "nose", oss.str(), "fragment matches");

			snf::http::uri uri_2 { "urn:example:animal:ferret:nose" };

			oss.str("");
			oss << uri_2.get_scheme();
			ASSERT_EQ(const std::string &, "urn", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_2.get_path();
			ASSERT_EQ(const std::string &, "example:animal:ferret:nose", oss.str(), "path matches");

			snf::http::uri uri_3 { "ldap://[2001:db8::7]/c=GB?objectClass?one" };

			oss.str("");
			oss << uri_3.get_scheme();
			ASSERT_EQ(const std::string &, "ldap", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_3.get_host();
			ASSERT_EQ(const std::string &, "[2001:db8::7]", oss.str(), "host matches");

			oss.str("");
			oss << uri_3.get_path();
			ASSERT_EQ(const std::string &, "/c=GB", oss.str(), "path matches");

			oss.str("");
			oss << uri_3.get_query();
			ASSERT_EQ(const std::string &, "objectClass?one", oss.str(), "query matches");

			snf::http::uri uri_4 { "mailto:fred@example.com" };

			oss.str("");
			oss << uri_4.get_scheme();
			ASSERT_EQ(const std::string &, "mailto", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_4.get_path();
			ASSERT_EQ(const std::string &, "fred@example.com", oss.str(), "path matches");

			snf::http::uri uri_5 { "foo://info.example.com?fred" };

			oss.str("");
			oss << uri_5.get_scheme();
			ASSERT_EQ(const std::string &, "foo", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_5.get_host();
			ASSERT_EQ(const std::string &, "info.example.com", oss.str(), "host matches");

			oss.str("");
			oss << uri_5.get_query();
			ASSERT_EQ(const std::string &, "fred", oss.str(), "query matches");

			snf::http::uri uri_6 { "news:comp.infosystems.www.servers.unix" };

			oss.str("");
			oss << uri_6.get_scheme();
			ASSERT_EQ(const std::string &, "news", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_6.get_path();
			ASSERT_EQ(const std::string &, "comp.infosystems.www.servers.unix", oss.str(), "path matches");

			snf::http::uri uri_7 { "tel:+1-816-555-1212" };

			oss.str("");
			oss << uri_7.get_scheme();
			ASSERT_EQ(const std::string &, "tel", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_7.get_path();
			ASSERT_EQ(const std::string &, "+1-816-555-1212", oss.str(), "path matches");

			snf::http::uri uri_8 { "telnet://192.0.2.16:80/" };

			oss.str("");
			oss << uri_8.get_scheme();
			ASSERT_EQ(const std::string &, "telnet", oss.str(), "scheme matches");

			oss.str("");
			oss << uri_8.get_host();
			ASSERT_EQ(const std::string &, "192.0.2.16", oss.str(), "host matches");

			oss.str("");
			oss << uri_8.get_port();
			ASSERT_EQ(const std::string &, "80", oss.str(), "host matches");

			test_merge("http://a/b/c/d;p?q");

		} catch (const std::runtime_error &ex) {
			std::cerr << "runtime error: " << ex.what() << std::endl;
			return false;
		}

		return true;
	}
};
