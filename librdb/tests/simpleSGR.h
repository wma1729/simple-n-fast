#include "error.h"
#include "rdb.h"

class SimpleSetGetRemove : public snf::tf::test
{
public:
	SimpleSetGetRemove() : snf::tf::test() {}
	~SimpleSetGetRemove() {}

	virtual const char *name() const
	{
		return "SimpleGSR";
	}

	virtual const char *description() const
	{
		return "Sets, gets, and removes 2 key/value pairs";
	}

	virtual bool execute(const snf::config *conf)
	{
		ASSERT_NE(const snf::config *, conf, nullptr, "check config");
		const char *dbPath = conf->get("DBPATH");
		ASSERT_NE(const char *, dbPath, nullptr, "get DBPATH from config");
		const char *dbName = conf->get("DBNAME");
		ASSERT_NE(const char *, dbName, nullptr, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(5);
		std::cerr << "dbpath = " << dbPath << std::endl;
		std::cerr << "dbname = " << dbName << std::endl;
		Rdb rdb(dbPath, dbName, 4096, 10, options);

		int retval = rdb.open();
		ASSERT_EQ(int, retval, E_ok, "rdb open");

		retval = rdb.set("abcd", 4, "012345", 6);

		ASSERT_EQ(int, retval, E_ok, "rdb set: key = abcd, value = 012345");

		char buf[32];
		int  buflen = (int)(sizeof(buf) - 1);

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(int, retval, E_ok, "rdb get: key = abcd");
		ASSERT_EQ(int, buflen, 6, "value length match");
		ASSERT_MEM_EQ(buf, "012345", 6, "value match");

		retval = rdb.set("ABCD", 4, "543210", 6);
		ASSERT_EQ(int, retval, E_ok, "rdb set: key = ABCD, value = 543210");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(int, retval, E_ok, "rdb get: key = ABCD");
		ASSERT_EQ(int, buflen, 6, "value length match");
		ASSERT_MEM_EQ(buf, "543210", 6, "value match");

		retval = rdb.remove("abcd", 4);
		ASSERT_EQ(int, retval, E_ok, "rdb remove: key = abcd");

		retval = rdb.remove("ABCD", 4);
		ASSERT_EQ(int, retval, E_ok, "rdb remove: key = ABCD");

		retval = rdb.get("abcd", 4, buf, &buflen);
		ASSERT_EQ(int, retval, E_not_found, "rdb get: key = abcd should return E_not_found");

		retval = rdb.get("ABCD", 4, buf, &buflen);
		ASSERT_EQ(int, retval, E_not_found, "rdb get: key = ABCD should return E_not_found");

		retval = rdb.close();
		ASSERT_EQ(int, retval, E_ok, "rdb close");

		return true;
	}
};
