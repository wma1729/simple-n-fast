#ifndef _SNF_TF_TEST_H_
#define _SNF_TF_TEST_H_

#include "common.h"
#include "config.h"
#include "perftimer.h"
#include "util.h"

namespace tf {

class Test
{
protected:
	local_time_t        st;
	PerformanceTimer    begin;
	PerformanceTimer    end;
	bool                failure;

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

		begin.record();

		bool passed = execute(config);

		end.record();

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
				end - begin);
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
