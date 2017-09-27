#include "error.h"
#include "rdb/rdb.h"

class SimpleSetGetRemove : public tf::Test
{
public:
	SimpleSetGetRemove()
		: tf::Test()
	{
	}

	~SimpleSetGetRemove()
	{
	}

	virtual const char *name() const
	{
		return "SimpleGSR";
	}

	virtual const char *description() const
	{
		return "Sets, gets, and removes a 2 key/value pairs";
	}

	virtual bool execute(const Config *config)
	{
		ASSERT_NE(config, 0, "check config");
		const char *dbPath = config->getString("DBPATH");
		ASSERT_NE(dbPath, 0, "get DBPATH from config");
		const char *dbName = config->getString("DBNAME");
		ASSERT_NE(dbName, 0, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(5);
		Rdb rdb(dbPath, dbName, 4096, 10, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open");

		retval = rdb.set("abcd", 4, "012345", 6);
		ASSERT_EQ(retval, E_ok, "rdb set (%s/%s)", "abcd", "012345");

		char buf[32];
		int  buflen = (int)(sizeof(buf) - 1);

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(retval, E_ok, "rdb get(%s)", "abcd");
		ASSERT_EQ(buflen, 6, "value length match");
		ASSERT_MEM_EQ(buf, "012345", 6, "value match");

		retval = rdb.set("ABCD", 4, "543210", 6);
		ASSERT_EQ(retval, E_ok, "rdb set (%s/%s)", "ABCD", "543210");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(retval, E_ok, "rdb get(%s)", "ABCD");
		ASSERT_EQ(buflen, 6, "value length match");
		ASSERT_MEM_EQ(buf, "543210", 6, "value match");

		retval = rdb.remove("abcd", 4);
		ASSERT_EQ(retval, E_ok, "rdb remove(%s)", "abcd");

		retval = rdb.remove("ABCD", 4);
		ASSERT_EQ(retval, E_ok, "rdb remove(%s)", "ABCD");

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(retval, E_not_found, "rdb get(%s) should return E_not_found", "abcd");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(retval, E_not_found, "rdb get(%s) should return E_not_found", "ABCD");

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "rdb close");

		return true;
	}
};
