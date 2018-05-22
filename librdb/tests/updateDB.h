#include "error.h"
#include "rdb.h"

struct test_value {
	char val[16];
	int  refcnt;
};

class MyUpdater : public Updater
{
private:
	char newValue[MAX_VALUE_LENGTH];
	int  newValueLen;

public:
	MyUpdater()
	{
		memset(newValue, 0, MAX_VALUE_LENGTH);
		newValueLen = 0;
	}

	// increment the reference count
	int update(const char *oval, int ovlen)
	{
		newValueLen = ovlen;
		memcpy(newValue, oval, ovlen);
		struct test_value *value = (struct test_value *)newValue;
		value->refcnt++;
		return 0;
	}

	// returns the updated record
	int getUpdatedValue(char *nval, int *nvlen)
	{
		*nvlen = newValueLen;
		if (*nvlen) {
			memcpy(nval, newValue, *nvlen);
		}
		return E_ok;
	}
};

class UpdateDB : public snf::tf::test
{
public:
	UpdateDB() : snf::tf::test() {}
	~UpdateDB() {}

	virtual const char *name() const
	{
		return "UpdateDB";
	}

	virtual const char *description() const
	{
		return "Sets, increments, and fetches key/value pair";
	}

	virtual bool execute(const snf::config *conf)
	{
		ASSERT_NE(conf, nullptr, "check config");
		const char *dbPath = conf->get("DBPATH");
		ASSERT_NE(dbPath, nullptr, "get DBPATH from config");
		const char *dbName = conf->get("DBNAME");
		ASSERT_NE(dbName, nullptr, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(1);
		Rdb rdb(dbPath, dbName, 4096, 10, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open");

		struct test_value value = { "dummydata", 1 };

		retval = rdb.set("dummykey", 8, (const char *)&value, (int)sizeof(value));
		ASSERT_EQ(retval, E_ok, "rdb set: key = dummykey");

		char buf1[32];
		int  buf1len = (int)(sizeof(buf1) - 1);
		char buf2[32];
		int  buf2len = (int)(sizeof(buf2) - 1);

		retval = rdb.get("dummykey", 8, buf1, &buf1len);
		ASSERT_EQ(retval, E_ok, "rdb get: key = dummykey");
		ASSERT_EQ(buf1len, (int)sizeof(value), "value length match");

		MyUpdater *mupdater = new MyUpdater();

		retval = rdb.set("dummykey", 8, "dummydata", 9, mupdater);
		ASSERT_EQ(retval, E_ok, "rdb update: key = dummykey");

		retval = rdb.get("dummykey", 8, buf1, &buf1len);
		ASSERT_EQ(retval, E_ok, "rdb get: key = dummykey");
		ASSERT_EQ(buf1len, (int)sizeof(value), "value length match");

		mupdater->getUpdatedValue(buf2, &buf2len);
		ASSERT_EQ(buf1len, buf2len, "db & updater - value length match");
		ASSERT_MEM_EQ(buf1, buf2, buf1len, "db & updater - value match");

		struct test_value *nvalue = (struct test_value *)buf1;
		ASSERT_EQ(nvalue->refcnt, 2, "reference count match");

		retval = rdb.remove("dummykey", 8);
		ASSERT_EQ(retval, E_ok, "rdb remove: key = dummykey");

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "rdb close");

		delete mupdater;

		return true;
	}
};
