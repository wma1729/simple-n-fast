#ifndef _SNF_TF_TESTSUITE_H_
#define _SNF_TF_TESTSUITE_H_

#include <vector>
#include "tf/test.h"

namespace tf {

extern Test *TestList[];

class TestSuite : public Test
{
private:
	std::string         sName;
	std::string         sDesc;
	local_time_t        et;
	int                 failCnt;
	std::vector<Test *> tests;

public:
	TestSuite(const std::string &sn, const std::string &sd)
		: sName(sn),
		  sDesc(sd),
		  failCnt(0)
	{
	}

	~TestSuite()
	{
		std::vector<Test *>::iterator I;
		for (I = tests.begin(); I != tests.end(); ++I) {
			Test *test = *I;
			delete test;
		}
		tests.clear();
	}

	void addTest(Test *test)
	{
		tests.push_back(test);
	}

	virtual const char *name() const
	{
		return sName.c_str();
	}

	virtual const char *description() const
	{
		return sDesc.c_str();
	}

	virtual bool execute(const Config *config)
	{
		GetLocalTime(&st);

		std::vector<Test *>::iterator I;
		for (I = tests.begin(); I != tests.end(); ++I) {
			Test *test = *I;
			if (test) {
				if (!test->run(config)) {
					failCnt++;
				}
				test->report();
			}
		}

		GetLocalTime(&et);

		failure = (failCnt != 0);

		return (failCnt == 0);
	}

	virtual void report()
	{
		char    buf[32];

		fprintf(stderr, "%s : %s\n", name(), description());
		fprintf(stderr, "Start Time   : %s\n",
				LocalTimeToString(&st, buf, sizeof(buf)));
		fprintf(stderr, "End Time     : %s\n",
				LocalTimeToString(&et, buf, sizeof(buf)));
		fprintf(stderr, "Failed Tests : %d/%d\n",
				failCnt, (int)tests.size());
		fprintf(stderr, "Status       : %s\n",
				failure ? "FAIL" : "PASS");
	}
};

} // namespace tf

#endif // _SNF_TF_TESTSUITE_H_
