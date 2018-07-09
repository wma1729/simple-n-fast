#include "logmgr.h"
#include "flogger.h"
#include <thread>
#include <chrono>

static int
usage(const char *progname)
{
	std::cerr
		<< progname << "[-sev <severity>] [-path <log_path>] [-mkpath]" << std::endl
		<< "[-mkpath] [-fmt <log_file_name_format>]" << std::endl
		<< "[-rotation_scheme <daily|by_size|daily_by_size>]" << std::endl
		<< "[-rotation_size <size_in_bytes]" <<std::endl
		<< "-num <num_of_logs> -interval <seconds_between_logs>" << std::endl;
	return 1;
}

static snf::log::severity
string_to_severity(const char *s)
{
	if ((s == nullptr) || (*s == '\0'))
		return snf::log::severity::all;

	if ((strcasecmp(s, "trc") == 0) || (strcasecmp(s, "trace") == 0))
		return snf::log::severity::trace;
	else if ((strcasecmp(s, "dbg") == 0) || (strcasecmp(s, "debug") == 0))
		return snf::log::severity::debug;
	else if ((strcasecmp(s, "inf") == 0) || (strcasecmp(s, "info") == 0))
		return snf::log::severity::info;
	else if ((strcasecmp(s, "wrn") == 0) || (strcasecmp(s, "warning") == 0))
		return snf::log::severity::warning;
	else if ((strcasecmp(s, "err") == 0) || (strcasecmp(s, "error") == 0))
		return snf::log::severity::error;

	return snf::log::severity::all;
}

int
main(int argc, const char **argv)
{
	char prog[MAXPATHLEN + 1];
	snf::log::severity sev = snf::log::severity::all;
	std::string log_path = ".";
	bool mk_log_path = false;
	std::string log_fname_fmt = "test_%D_%N.log";
	snf::log::rotation::scheme scheme = snf::log::rotation::scheme::none;
	int64_t rsize = 1024;
	int num = 100;
	int interval = 1;

	ENABLE_LEAK_CHECK;

	snf::basename(prog, MAXPATHLEN + 1, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcasecmp(argv[i], "-sev") == 0) {
			++i;
			if (argv[i]) {
				sev = string_to_severity(argv[i]);
			} else {
				std::cerr << "argument to -sev missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-path") == 0) {
			++i;
			if (argv[i]) {
				log_path = argv[i];
			} else {
				std::cerr << "argument to -path missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-mkpath") == 0) {
			mk_log_path = true;
		} else if (strcasecmp(argv[i], "-fmt") == 0) {
			++i;
			if (argv[i]) {
				log_fname_fmt = argv[i];
			} else {
				std::cerr << "argument to -fmt missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-rotation_scheme") == 0) {
			++i;
			if (argv[i]) {
				if (strcasecmp(argv[i], "daily") == 0) {
					scheme = snf::log::rotation::scheme::daily;
				} else if (strcasecmp(argv[i], "by_size") == 0) {
					scheme = snf::log::rotation::scheme::by_size;
				} else if (strcasecmp(argv[i], "daily_by_size") == 0) {
					scheme =
						snf::log::rotation::scheme::daily |
						snf::log::rotation::scheme::by_size;
				} else {
					std::cerr << "invalid argument (" << argv[i]
						<< ") to -rotation_scheme" << std::endl;
					return usage(prog);
				}
			} else {
				std::cerr << "argument to -rotation_scheme missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp(argv[i], "-rotation_size") == 0) {
			++i;
			if (argv[i]) {
				rsize = static_cast<int64_t>(std::stoll(argv[i]));
			} else {
				std::cerr << "argument to -rotation_scheme missing" << std::endl;
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

	snf::log::file_logger *flog = DBG_NEW snf::log::file_logger(log_path, sev);
	flog->make_path(mk_log_path);
	flog->set_name_format(log_fname_fmt);
	if (scheme != snf::log::rotation::scheme::none) {
		snf::log::rotation *rotation = DBG_NEW snf::log::rotation(scheme);
		if (rotation->by_size()) {
			rotation->size(rsize);
		}
		flog->set_rotation(rotation);
	}

	snf::log::manager::instance().add_logger(flog);

	for (int i = 0; i < num; ++i) {
		ERROR_STRM(nullptr, nullptr)
			<< i << ": error message"
			<< snf::log::record::endl;
		WARNING_STRM(nullptr, nullptr)
			<< i << ": warning message"
			<< snf::log::record::endl;
		INFO_STRM(nullptr, nullptr)
			<< i << ": info message"
			<< snf::log::record::endl;
		DEBUG_STRM(nullptr, nullptr)
			<< i << ": debug message"
			<< snf::log::record::endl;
		TRACE_STRM(nullptr, nullptr)
			<< i << ": trace message"
			<< snf::log::record::endl;
		std::this_thread::sleep_for(std::chrono::seconds { interval });
	}

	return 0;
}
