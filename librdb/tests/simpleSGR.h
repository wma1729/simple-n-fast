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
		return "Sets, gets, and removes a single key/value pair";
	}

	virtual bool execute(const Config *config)
	{
		ASSERT_NE(config, 0, "config not set");
		const char *dbPath = config->getString("DBPATH");
		ASSERT_NE(dbPath, 0, "DBPATH is not set in config");
		const char *dbName = config->getString("DBNAME");
		ASSERT_NE(dbName, 0, "DBNAME is not set in config");

		RdbOptions options;
		options.setMemoryUsage(5);
		Rdb rdb(dbPath, dbName, 4096, 10, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "db open failed");

		retval = rdb.set("abcd", 4, "012345", 6);
		ASSERT_EQ(retval, E_ok, "db set (abcd/012345) failed");

		char buf[32];
		int  buflen = (int)(sizeof(buf) - 1);

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(retval, E_ok, "db get(abcd) failed");
		ASSERT_EQ(buflen, 6, "value length mismatch");
		ASSERT_SEQ(buf, "012345", 6, "value mismatch");

		retval = rdb.set("ABCD", 4, "543210", 6);
		ASSERT_EQ(retval, E_ok, "db set (ABCD/543210) failed");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(retval, E_ok, "db get(ABCD) failed");
		ASSERT_EQ(buflen, 6, "value length mismatch");
		ASSERT_SEQ(buf, "543210", 6, "value mismatch");

		retval = rdb.remove("abcd", 4);
		ASSERT_EQ(retval, E_ok, "db remove(abcd) failed");

		retval = rdb.remove("ABCD", 4);
		ASSERT_EQ(retval, E_ok, "db remove(ABCD) failed");

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(retval, E_not_found, "db get(abcd) did not return E_not_found");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(retval, E_not_found, "db get(ABCD) did not return E_not_found");

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "db close failed");

		return true;
	}
};
