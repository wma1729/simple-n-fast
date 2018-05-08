#ifndef _SNF_TF_TEST_H_
#define _SNF_TF_TEST_H_

#include "common.h"
#include "config.h"
#include <chrono>

namespace tf {

using namespace std::chrono;

class Test
{
protected:
	local_time_t st;
	time_point<high_resolution_clock> begin;
	time_point<high_resolution_clock> end;
	bool failure;

public:
	Test()
	{
		failure = false;
	}

	virtual ~Test()
	{
	}

	virtual const char *name() const = 0;
	virtual const char *description() const = 0;
	virtual bool execute(const Config *) = 0;

	virtual bool run(const Config *config)
	{
		GetLocalTime(&st);

		begin = high_resolution_clock::now();

		bool passed = execute(config);

		end = high_resolution_clock::now();

		failure = !passed;

		return passed;
	}

	virtual void report()
	{
		char    buf[32];

		fprintf(stderr, "%s : %s\n", name(), description());
		fprintf(stderr, "Start Time   : %s\n",
				LocalTimeToString(&st, buf, sizeof(buf)));
		fprintf(stderr, "Elapsed Time : %" PRId64 " micro-seconds\n",
				duration_cast<microseconds>(end - begin).count());
		fprintf(stderr, "Status       : %s\n",
				failure ? "FAIL" : "PASS");
	}

	virtual bool failed()
	{
		return failure;
	}
};

} // namespace tf

#endif // _SNF_TF_TEST_H_
