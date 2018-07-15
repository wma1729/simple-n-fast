#include "rdb.h"
#include "logmgr.h"
#include "flogger.h"

static bool   Verbosity;

static int
usage(const char *prog)
{
	std::cerr
		<< prog
		<< " [-get|-set|-del|-rebuild] -path <db_path> -name <db_name>" << std::endl
		<< "        -key <key> [-value <value>]" << std::endl
		<< "        [-htsize <hash_table_size>] [-pgsize <page_size>]" << std::endl
		<< "        [-memusage <%_of_memory>] [-syncdf <0|1>]" << std::endl
		<< "        [-syncif <0|1>] [-logpath <log_path>]" << std::endl;
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
				std::cerr << "missing argument to -path" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-name", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				name = argv[i];
			} else {
				std::cerr << "missing argument to -name" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-htsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				htSize = atoi(argv[i]);
			} else {
				std::cerr << "missing argument to -htsize" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-pgsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				pgSize = atoi(argv[i]);
			} else {
				std::cerr << "missing argument to -pgsize" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-memusage", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				if (dbOpt.setMemoryUsage(atoi(argv[i])) != E_ok) {
					std::cerr
						<< "invalid memory usage ("
						<< argv[i] << ")" << std::endl;
					return 1;
				}
			} else {
				std::cerr << "missing argument to -memusage" << std::endl;
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
				std::cerr << "missing argument to -syncdf" << std::endl;
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
				std::cerr << "missing argument to -syncif" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-pgsize", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				pgSize = atoi(argv[i]);
			} else {
				std::cerr << "missing argument to -pgsize" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-key", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				key = argv[i];
			} else {
				std::cerr << "missing argument to -key" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-value", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				value = argv[i];
			} else {
				std::cerr << "missing argument to -value" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-logpath", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				logPath = argv[i];
			} else {
				std::cerr << "missing argument to -logpath" << std::endl;
				return usage(prog);
			}
		} else if (strcmp("-v", argv[i]) == 0) {
			Verbosity = true;
		} else {
			return usage(prog);
		}
	}

	if (!logPath.empty()) {
		snf::log::severity sev = snf::log::severity::info;
		if (Verbosity)
			sev = snf::log::severity::trace;

		snf::log::file_logger *flog = DBG_NEW snf::log::file_logger {
						logPath,
						sev };
		flog->make_path(true);
		snf::log::manager::instance().add_logger(flog);
	}

	if ((cmd == NIL) && !rebuild) {
		std::cerr << "one of [-get|-set|-del|-rebuild] must be specified" << std::endl;
		return usage(prog);
	}

	if (path.empty()) {
		std::cerr << "database path not specified" << std::endl;
		return usage(prog);
	}

	if (name.empty()) {
		std::cerr << "database name not specified" << std::endl;
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
			std::cerr << "rebuild failed with status" << retval << std::endl;
			return 1;
		}

		return 0;
	}

	if (key.empty()) {
		std::cerr << "key not specified" << std::endl;
		return usage(prog);
	}

	if ((cmd == SET) && value.empty()) {
		std::cerr << "value not specified" << std::endl;
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
					std::cout << val << std::endl;
				} else if (retval == E_not_found) {
					std::cout << key << " not found" << std::endl;
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
					std::cout << key << " is set to " << value << std::endl;
				}
				break;

			case DEL:
			default:
				retval = rdb.remove(key.c_str(), (int) key.size());
				if (retval == E_ok) {
					std::cout << key << " is removed" << std::endl;
				}
				break;
		}

		rdb.close();
	}

	if (retval != E_ok) {
		std::cerr << "operation failed with status " << retval << std::endl;
		retval = 1;
	}

	return retval;
}
