#include <map>
#include "error.h"
#include "filesystem.h"
#include "rdb.h"

extern void GenKeyValue(char *, char *, int);

class RebuildDB : public snf::tf::Test
{
public:
	RebuildDB() : snf::tf::Test() {}
	~RebuildDB() {}

	virtual const char *name() const
	{
		return "RebuildDB";
	}

	virtual const char *description() const
	{
		return "Rebuild database";
	}

	virtual bool execute(const snf::config *conf)
	{
		ASSERT_NE(conf, 0, "check config");
		const char *dbPath = conf->get("DBPATH");
		ASSERT_NE(dbPath, 0, "get DBPATH from config");
		const char *dbName = conf->get("DBNAME");
		ASSERT_NE(dbName, 0, "get DBNAME from config");

		RdbOptions options;
		Rdb rdb(dbPath, dbName, 1024, 5, options);

		int retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open");

		struct KV {
			const char *key;
			int klen;
			const char *value;
			int vlen;
		};

		KV kvpair[] = {
			{ "a", 1, "A", 1 },
			{ "b", 1, "B", 1 },
			{ "c", 1, "C", 1 },
			{ "d", 1, "D", 1 },
			{ "aa", 2, "AA", 2 },
			{ "bb", 2, "BB", 2 },
			{ "cc", 2, "CC", 2 },
			{ "dd", 2, "DD", 2 },
			{ "aaa", 3, "AAA", 3 },
			{ "bbb", 3, "BBB", 3 },
			{ "ccc", 3, "CCC", 3 },
			{ "ddd", 3, "DDD", 3 },
			{ "aaaa", 4, "AAAA", 4 },
			{ "bbbb", 4, "BBBB", 4 },
			{ "cccc", 4, "CCCC", 4 },
			{ "dddd", 4, "DDDD", 4 },
			{ 0, -1, 0, -1 }
		};

		for (int i = 0; i < 16; ++i) {
			retval = rdb.set(kvpair[i].key, kvpair[i].klen,
					kvpair[i].value, kvpair[i].vlen);

			m_strm << "rdb set: key = " << kvpair[i].key
				<< ", value = " << kvpair[i].value;
			ASSERT_EQ(retval, E_ok, m_strm.str());
			m_strm.str("");
		}

		retval = rdb.remove("a", 1);
		ASSERT_EQ(retval, E_ok, "rdb remove: key = a");

		retval = rdb.remove("bb", 2);
		ASSERT_EQ(retval, E_ok, "rdb remove: key = bb");

		retval = rdb.remove("ccc", 3);
		ASSERT_EQ(retval, E_ok, "rdb remove: key = ccc");

		retval = rdb.remove("dddd", 4);
		ASSERT_EQ(retval, E_ok, "rdb remove: key = dddd");
		m_strm.str("");

		retval = rdb.close();
		ASSERT_EQ(retval, E_ok, "rdb close");

		retval = rdb.rebuild();
		ASSERT_EQ(retval, E_ok, "rdb rebuild");

		char dbpath[MAXPATHLEN + 1];
		char fdppath[MAXPATHLEN + 1];

		snprintf(dbpath, MAXPATHLEN, "%s%c%s.db", dbPath, PATH_SEP, dbName);
		snprintf(fdppath, MAXPATHLEN, "%s%c%s.fdp", dbPath, PATH_SEP, dbName);

		int64_t dbsize = snf::fs::size(dbpath);
		int64_t expsize = sizeof(value_page_t) * 12;
		int64_t fdpsize = snf::fs::size(fdppath);

		ASSERT_EQ(fdpsize, 8, "fdp size match");
		ASSERT_EQ(dbsize, expsize, "db size match");

		retval = rdb.open();
		ASSERT_EQ(retval, E_ok, "rdb open again");

		for (int i = 0; i < 16; ++i) {
			retval = rdb.remove(kvpair[i].key, kvpair[i].klen);
			if ((strcmp("a", kvpair[i].key) == 0) ||
				(strcmp("bb", kvpair[i].key) == 0) ||
				(strcmp("ccc", kvpair[i].key) == 0) ||
				(strcmp("dddd", kvpair[i].key) == 0)) {
				m_strm << "already deleted key" << kvpair[i].key;
				ASSERT_EQ(retval, E_not_found, m_strm.str());
				m_strm.str("");
			} else {
				m_strm << "delete key " << kvpair[i].key;
				ASSERT_EQ(retval, E_ok, m_strm.str());
				m_strm.str("");
			}
		}

		return true;
	}
};
