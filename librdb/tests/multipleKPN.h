#include <map>
#include "error.h"
#include "rdb/rdb.h"

extern void GenKeyValue(char *, char *, int);

class MultipleKeyPageNodes : public tf::Test
{
public:
	MultipleKeyPageNodes()
		: tf::Test()
	{
	}

	~MultipleKeyPageNodes()
	{
	}

	virtual const char *name() const
	{
		return "MultipleKPN";
	}

	virtual const char *description() const
	{
		return "Multiple key page nodes with very few hash table entries";
	}

	virtual bool execute(const Config *config)
	{
		ASSERT_NE(config, 0, "check config");
		const char *dbPath = config->getString("DBPATH");
		ASSERT_NE(dbPath, 0, "get DBPATH from config");
		const char *dbName = config->getString("DBNAME");
		ASSERT_NE(dbName, 0, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(2);
		options.syncDataFile(false);
		Rdb rdb(dbPath, dbName, 1024, 5, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open");

		char key[33] = { 0 };
		char val[33] = { 0 };
		char outbuf[33] = { 0 };
		int  outlen = 32;
		std::map<std::string, std::string> kvPair;

		for (int i = 0; i < 10000; ++i) {
			GenKeyValue(key, val, 32);
			kvPair[key] = val;

			retval = rdb.set(key, 32, val, 32);
			ASSERT_EQ(retval, E_ok, "rdb set (%s/%s)", key, val);
		}

		std::map<std::string, std::string>::iterator I;
		for (I = kvPair.begin(); I != kvPair.end(); ++I) {
			retval = rdb.get(I->first.c_str(), 32, outbuf, &outlen);
			ASSERT_EQ(retval, E_ok, "rdb get(%s)", I->first.c_str());
			ASSERT_EQ(outlen, 32, "value length match");
			ASSERT_MEM_EQ(outbuf, I->second.c_str(), 32, "value match");

			retval = rdb.remove(I->first.c_str(), 32);
			ASSERT_EQ(retval, E_ok, "rdb remove(%s)", I->first.c_str());

			retval = rdb.get(I->first.c_str(), 32, outbuf, &outlen);
			ASSERT_EQ(retval, E_not_found, "rdb get(%s) should return E_not_found",
				I->first.c_str());
		}

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "rdb close");

		return true;
	}
};
