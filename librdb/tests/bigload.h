#include <map>
#include "error.h"
#include "rdb/rdb.h"

extern void GenKeyValue(char *, char *, int);

class BigLoad : public snf::tf::Test
{
public:
	BigLoad() : snf::tf::Test() {}
	~BigLoad() {}

	virtual const char *name() const
	{
		return "BigLoad";
	}

	virtual const char *description() const
	{
		return "Adds 2880000 million keys";
	}

	virtual bool execute(const snf::config *conf)
	{
		ASSERT_NE(conf, 0, "check config");
		const char *dbPath = conf->get("DBPATH");
		ASSERT_NE(dbPath, 0, "get DBPATH from config");
		const char *dbName = conf->get("DBNAME");
		ASSERT_NE(dbName, 0, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(75);
		options.syncDataFile(false);
		Rdb rdb(dbPath, dbName, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open");

		char key[33] = { 0 };
		char val[33] = { 0 };

		/*
		 * 2880000 = 800 * 60 * 60
		 * Assuming that we can set 800 entries
		 * in a second on a system with 16GB
		 * memory and 4 core CPU.
		 */
		for (int i = 0; i < 2880000; ++i) {
			GenKeyValue(key, val, 32);
			retval = rdb.set(key, 32, val, 32);

			m_strm << "rdb set: key = " << key << ", value = " << val;
			ASSERT_EQ(retval, E_ok, m_strm.str());
			m_strm.str("");
		}

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "rdb close");

		return true;
	}
};
