#include "log.h"
#include "rdb/rdb.h"

extern Logger *TheLogger;
extern bool   TheVerbosity;

static int
usage(const char *prog)
{
	fprintf(stderr, "%s [-get|-set|-del|-rebuild] -path <db_path> -name <db_name>\n", prog);
	fprintf(stderr, "        -key <key> [-value <value>]\n");
	fprintf(stderr, "        [-htsize <hash_table_size>] [-pgsize <page_size>]\n");
	fprintf(stderr, "        [-memusage <%%_of_memory>] [-syncdf <0|1>]\n");
	fprintf(stderr, "        [-syncif <0|1>] [-logpath <log_path>]\n");
	return 1;
}

int
main(int argc, const char **argv)
{
	int retval = E_ok;
	op_t cmd = NIL;
	std::string path;
	std::string name;
	std::string key;
	std::string value;
	std::string logPath;
	int htSize = -1;
	int pgSize = -1;
	RdbOptions dbOpt;
	int vlen;
	char val[MAX_VALUE_LENGTH + 1];
	char prog[MAXPATHLEN + 1];
	bool rebuild = false;

	snf::basename(prog, MAXPATHLEN + 1, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcmp("-get", argv[i]) == 0) {
			cmd = GET;
		} else if (strcmp("-set", argv[i]) == 0) {
			cmd = SET;
		} else if (strcmp("-del", argv[i]) == 0) {
			cmd = DEL;
		} else if (strcmp("-rebuild", argv[i]) == 0) {
			rebuild = true;
		} else if (strcmp("-path", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				path = argv[i];
			} else {
				fprintf(stderr, "missing argument to -path\n");
				return usage(prog);
			}
		} else if (strcmp("-name", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				name = argv[i];
			} else {
				fprintf(stderr, "missing argument to -name\n");
				return usage(prog);
			}
		} else if (strcmp("-htsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				htSize = atoi(argv[i]);
			} else {
				fprintf(stderr, "missing argument to -htsize\n");
				return usage(prog);
			}
		} else if (strcmp("-pgsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				pgSize = atoi(argv[i]);
			} else {
				fprintf(stderr, "missing argument to -pgsize\n");
				return usage(prog);
			}
		} else if (strcmp("-memusage", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				if (dbOpt.setMemoryUsage(atoi(argv[i])) != E_ok) {
					fprintf(stderr, "invalid memory usage (%s)\n",
						argv[i]);
					return 1;
				}
			} else {
				fprintf(stderr, "missing argument to -memusage\n");
				return usage(prog);
			}
		} else if (strcmp("-syncdf", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				if (atoi(argv[i]) == 0) {
					dbOpt.syncDataFile(false);
				} else {
					dbOpt.syncDataFile(true);
				}
			} else {
				fprintf(stderr, "missing argument to -syncdf\n");
				return usage(prog);
			}
		} else if (strcmp("-syncif", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				if (atoi(argv[i]) == 1) {
					dbOpt.syncIndexFile(true);
				} else {
					dbOpt.syncIndexFile(false);
				}
			} else {
				fprintf(stderr, "missing argument to -syncif\n");
				return usage(prog);
			}
		} else if (strcmp("-pgsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				pgSize = atoi(argv[i]);
			} else {
				fprintf(stderr, "missing argument to -pgsize\n");
				return usage(prog);
			}
		} else if (strcmp("-key", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				key = argv[i];
			} else {
				fprintf(stderr, "missing argument to -key\n");
				return usage(prog);
			}
		} else if (strcmp("-value", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				value = argv[i];
			} else {
				fprintf(stderr, "missing argument to -value\n");
				return usage(prog);
			}
		} else if (strcmp("-logpath", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				logPath = argv[i];
			} else {
				fprintf(stderr, "missing argument to -logpath\n");
				return usage(prog);
			}
		} else if (strcmp("-v", argv[i]) == 0) {
			TheVerbosity = true;
		} else {
			return usage(prog);
		}
	}

	if (!logPath.empty()) {
		TheLogger = DBG_NEW FileLogger(logPath.c_str(), TheVerbosity);
	}

	if ((cmd == NIL) && !rebuild) {
		fprintf(stderr, "one of [-get|-set|-del|-rebuild] must be specified\n");
		return usage(prog);
	}

	if (path.empty()) {
		fprintf(stderr, "database path not specified\n");
		return usage(prog);
	}

	if (name.empty()) {
		fprintf(stderr, "database name not specified\n");
		return usage(prog);
	}

	if (rebuild) {
		Rdb rdb(path, name, dbOpt);

		if (pgSize != -1)
			rdb.setKeyPageSize(pgSize);

		if (htSize != -1)
			rdb.setHashTableSize(htSize);

		retval = rdb.rebuild();
		if (retval != E_ok) {
			fprintf(stderr, "rebuild failed with status %d\n", retval);
			return 1;
		}

		return 0;
	}

	if (key.empty()) {
		fprintf(stderr, "key not specified\n");
		return usage(prog);
	}

	if ((cmd == SET) && value.empty()) {
		fprintf(stderr, "value not specified\n");
		return usage(prog);
	}

	Rdb rdb(path, name, dbOpt);

	if (pgSize != -1)
		rdb.setKeyPageSize(pgSize);

	if (htSize != -1)
		rdb.setHashTableSize(htSize);

	retval = rdb.open();
	if (retval == E_ok) {
		switch (cmd) {
			case GET:
				vlen = MAX_VALUE_LENGTH;
				retval = rdb.get(
						key.c_str(),
						(int)key.size(),
						val,
						&vlen);
				if (retval == E_ok) {
					val[vlen] = '\0';
					fprintf(stdout, "%s\n", val);
				} else if (retval == E_not_found) {
					fprintf(stdout, "%s not found\n", key.c_str());
					retval = E_ok;
				}
				break;

			case SET:
				retval = rdb.set(
						key.c_str(),
						(int)key.size(),
						value.c_str(),
						(int)value.size());
				if (retval == E_ok) {
					fprintf(stdout, "%s is set to %s\n",
						key.c_str(), value.c_str());
				}
				break;

			case DEL:
			default:
				retval = rdb.remove(key.c_str(), (int) key.size());
				if (retval == E_ok) {
					fprintf(stdout, "%s is removed\n",
						key.c_str());
				}
				break;
		}

		rdb.close();
	}

	if (retval != E_ok) {
		fprintf(stderr, "operation failed with status %d\n", retval);
		retval = 1;
	}

	return retval;
}
