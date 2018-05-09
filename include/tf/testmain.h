#ifndef _SNF_TF_TESTMAIN_H_
#define _SNF_TF_TESTMAIN_H_

#include "tf/testsuite.h"
#include <cctype>
#include <vector>
#include "log.h"
#include "error.h"

extern Logger   *TheLogger;
extern bool     TheVerbosity;

static int
usage(const char *prog)
{
	fprintf(stderr,
		"%s [-name <suite_name>] [-desc <suite_description>]\n"
		"[-config <config_file>] [-tests <comma_separated_list_of_tests>]\n"
		"[-logpath <log_path>] [-v] [-h]\n",
		prog);
	return 1;
}

static tf::Test *
find_test(const char *tname)
{
	for (int i = 0; ; ++i) {
		if (tf::TestList[i]) {
			if (strcasecmp(tname, tf::TestList[i]->name()) == 0) {
				return tf::TestList[i];
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
	int  retval = E_ok;
	char prog[MAXPATHLEN + 1];
	char buf[MAXPATHLEN + 1];
	const char *sn = 0;
	const char *sd = 0;
	const char *cf = 0;
	const char *tl = 0;
	const char *lp = 0;
	const char *t = 0;
	Config *config = 0;

#if defined(_WIN32) && defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);  
#endif

	GetBaseName(prog, MAXPATHLEN + 1, argv[0], true);

	for (int i = 1; i < argc; ++i) {
		if (strcasecmp("-name", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				sn = argv[i];
			} else {
				fprintf(stderr, "argument to -name missing\n");
				return usage(prog);
			}
		} else if (strcasecmp("-desc", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				sd = argv[i];
			} else {
				fprintf(stderr, "argument to -config missing\n");
				return usage(prog);
			}
		} else if (strcasecmp("-config", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				cf = argv[i];
			} else {
				fprintf(stderr, "argument to -config missing\n");
				return usage(prog);
			}
		} else if (strcasecmp("-tests", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				tl = argv[i];
			} else {
				fprintf(stderr, "argument to -tests missing\n");
				return usage(prog);
			}
		} else if (strcasecmp("-logpath", argv[i]) == 0) {
			++i;
			if (argv[i]) {
				lp = argv[i];
			} else {
				fprintf(stderr, "argument to -logpath missing\n");
				return usage(prog);
			}
		} else if (strcasecmp("-v", argv[i]) == 0) {
			TheVerbosity = true;
		} else if (strcasecmp("-h", argv[i]) == 0) {
			usage(prog);
			retval = 0;
			break;
		} else {
			fprintf(stderr, "invalid argument %s\n", argv[i]);
			return usage(prog);
		}
	}

	if (sn == 0) {
		sn = "UnnamedTestSuite";
	}

	if (sd == 0) {
		sd = "No description available.";
	}

	if (lp) {
		TheLogger = DBG_NEW FileLogger(lp, TheVerbosity);
	}

	if (cf) {
		config = DBG_NEW Config(cf);
		if (retval != E_ok) {
			fprintf(stderr, "failed to read config file %s\n", cf);
			return 1;
		}
	}

	tf::TestSuite suite(sn, sd);

	if (tl) {
		int i = 0;
		t = tl;

		while (*t) {
			if (isspace(*t) || (*t == ',')) {
				if (i > 0) {
					buf[i] = '\0';
					tf::Test *test = find_test(buf);
					if (test) {
						suite.addTest(test);
					}
					i = 0;
				}
			} else {
				buf[i++] = *t;
			}
			t++;
		}

		if (i > 0) {
			buf[i] = '\0';
			tf::Test *test = find_test(buf);
			if (test) {
				suite.addTest(test);
			}
		}
	} else {
		for (int i = 0; ; ++i) {
			if (tf::TestList[i]) {
				suite.addTest(tf::TestList[i]);
			} else {
				break;
			}
		}
	}

	suite.run(config);
	suite.report();

	if (config)
		delete config;

	if (TheLogger)
		delete TheLogger;

	return suite.failed() ? 1 : 0;
	
}

#endif // _SNF_TF_TESTMAIN_H_
