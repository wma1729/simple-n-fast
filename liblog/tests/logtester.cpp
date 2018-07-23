#include "logmgr.h"
#include "flogger.h"
#include <thread>
#include <chrono>

static int
usage(const char *progname)
{
	std::cerr
		<< progname << " -conf <log_conf>" << std::endl
		<< "    -num <num_of_logs> -interval <seconds_between_logs>" << std::endl;
	return 1;
}

int
main(int argc, const char **argv)
{
	char prog[MAXPATHLEN + 1];
	std::string logconf;
	int num = 100;
	int interval = 0;

	ENABLE_LEAK_CHECK;

	snf::basename(prog, MAXPATHLEN + 1, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcasecmp(argv[i], "-conf") == 0) {
			++i;
			if (argv[i]) {
				logconf = argv[i];
			} else {
				std::cerr << "argument to -conf missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-num") == 0) {
			++i;
			if (argv[i]) {
				num = std::stoi(argv[i]);
			} else {
				std::cerr << "argument to -num missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-interval") == 0) {
			++i;
			if (argv[i]) {
				interval = std::stoi(argv[i]);
			} else {
				std::cerr << "argument to -interval missing" << std::endl;
				return usage(prog);
			}
		} else {
			std::cerr << "invalid argument (" << argv[i] << ") specified" << std::endl;
			return usage(prog);
		}
	}

	if (logconf.empty()) {
		std::cerr << "log configuration must be specified using -conf" << std::endl;
		return usage(prog);
	}

	snf::log::manager::instance().load(logconf, "logtester");

	for (int i = 0; i < num; ++i) {
		ERROR_STRM(nullptr)
			<< i << ": error message"
			<< snf::log::record::endl;
		WARNING_STRM(nullptr)
			<< i << ": warning message"
			<< snf::log::record::endl;
		INFO_STRM(nullptr)
			<< i << ": info message"
			<< snf::log::record::endl;
		DEBUG_STRM(nullptr)
			<< i << ": debug message"
			<< snf::log::record::endl;
		TRACE_STRM(nullptr)
			<< i << ": trace message"
			<< snf::log::record::endl;

		if (interval)
			std::this_thread::sleep_for(std::chrono::seconds { interval });
	}

	return 0;
}
