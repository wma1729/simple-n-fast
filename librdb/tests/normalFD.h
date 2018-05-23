#include <map>
#include "error.h"
#include "rdb.h"

extern void GenKeyValue(char *, char *, int);

class NormalFairDistribution : public snf::tf::test
{
public:
	NormalFairDistribution() : snf::tf::test() {}
	~NormalFairDistribution() {}

	virtual const char *name() const
	{
		return "NormalFD";
	}

	virtual const char *description() const
	{
		return "Normal scenario with fair hash table distribution";
	}

	virtual bool execute(const snf::config *conf)
	{
		ASSERT_NE(const snf::config *, conf, nullptr, "check config");
		const char *dbPath = conf->get("DBPATH");
		ASSERT_NE(const char *, dbPath, nullptr, "get DBPATH from config");
		const char *dbName = conf->get("DBNAME");
		ASSERT_NE(const char *, dbName, nullptr, "get DBNAME from config");

		RdbOptions options;
		options.setMemoryUsage(50);
		options.syncDataFile(false);
		Rdb rdb(dbPath, dbName, 4096, 1000, options);

		int retval = rdb.open();
		ASSERT_EQ(int, retval, E_ok, "rdb open");

		char key[33] = { 0 };
		char val[33] = { 0 };
		char outbuf[33] = { 0 };
		int  outlen = 32;
		std::map<std::string, std::string> kvPair;

		for (int i = 0; i < 10000; ++i) {
			GenKeyValue(key, val, 32);
			kvPair[key] = val;

			retval = rdb.set(key, 32, val, 32);

			m_strm << "rdb set: key = " << key << ", value = " << val;
			ASSERT_EQ(int, retval, E_ok, m_strm.str());
			m_strm.str("");
		}

		std::map<std::string, std::string>::iterator I;
		for (I = kvPair.begin(); I != kvPair.end(); ++I) {
			retval = rdb.get(I->first.c_str(), 32, outbuf, &outlen);

			m_strm << "rdb get: key = " << I->first;
			ASSERT_EQ(int, retval, E_ok, m_strm.str());
			m_strm.str("");

			ASSERT_EQ(int, outlen, 32, "value length match");
			ASSERT_MEM_EQ(outbuf, I->second.c_str(), 32, "value match");

			retval = rdb.remove(I->first.c_str(), 32);

			m_strm << "rdb remove: key = " << I->first;
			ASSERT_EQ(int, retval, E_ok, m_strm.str());
			m_strm.str("");

			retval = rdb.get(I->first.c_str(), 32, outbuf, &outlen);

			m_strm << "rdb get: key = " << I->first << " should return E_not_found";
			ASSERT_EQ(int, retval, E_not_found, m_strm.str());
			m_strm.str("");
		}

		retval = rdb.close();
		ASSERT_EQ(int, retval, E_ok, "rdb close");

		return true;
	}
};
