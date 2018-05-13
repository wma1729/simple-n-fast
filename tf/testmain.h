#ifndef _SNF_TF_TESTMAIN_H_
#define _SNF_TF_TESTMAIN_H_

#include <cctype>
#include "test.h"

static int
usage(const char *prog)
{
	std::cerr
		<< prog
		<< "[-name <suite_name>] [-desc <suite_description>]" << std::endl
		<< "[-config <config_file>] [-tests <comma_separated_list_of_tests>]" << std::endl
		<< "[-h]" << std::endl;
	return 1;
}

static snf::tf::Test *
find_test(const std::string &tname)
{
	for (int i = 0; ; ++i) {
		if (snf::tf::TestList[i]) {
			if (strcasecmp(tname.c_str(), snf::tf::TestList[i]->name()) == 0) {
				return snf::tf::TestList[i];
			}
		} else {
			break;
		}
	}

	return 0;
}

int
main(int argc, const char **argv)
{
	char prog[MAXPATHLEN + 1];
	const char *sn = 0;
	const char *sd = 0;
	const char *cf = 0;
	const char *tl = 0;
	snf::config *conf = 0;

	ENABLE_LEAK_CHECK;

	snf::basename(prog, MAXPATHLEN + 1, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcasecmp("-name", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				sn = argv[i];
			} else {
				std::cerr << "argument to -name missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp("-desc", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				sd = argv[i];
			} else {
				std::cerr << "argument to -config missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp("-config", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				cf = argv[i];
			} else {
				std::cerr << "argument to -config missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp("-tests", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				tl = argv[i];
			} else {
				std::cerr << "argument to -tests missing" << std::endl;
				return usage(prog);
			}
		} else if (strcasecmp("-h", argv[i]) == 0) {
			usage(prog);
			break;
		} else {
			std::cerr << "invalid argument " << argv[i] << std::endl;
			return usage(prog);
		}
	}

	if (sn == 0) {
		sn = "UnnamedTestSuite";
	}

	if (sd == 0) {
		sd = "No description available.";
	}

	if (cf) {
		conf = DBG_NEW snf::config(cf);
	}

	snf::tf::TestSuite suite(sn, sd);

	if (tl) {
		std::stringstream ss(tl);

		while (ss) {
			std::string s;
			std::getline(ss, s, ',');
			snf::tf::Test *test = find_test(snf::trim(s));
			if (test) {
				suite.addTest(test);
			}
		}
	} else {
		for (int i = 0; ; ++i) {
			if (snf::tf::TestList[i]) {
				suite.addTest(snf::tf::TestList[i]);
			} else {
				break;
			}
		}
	}

	suite.run(conf);
	suite.report();

	if (conf)
		delete conf;

	return suite.failed() ? 1 : 0;
}

#endif // _SNF_TF_TESTMAIN_H_
