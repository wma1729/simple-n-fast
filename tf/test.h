#ifndef _SNF_TF_TEST_H_
#define _SNF_TF_TEST_H_

#include "common.h"
#include "config.h"
#include "timeutil.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace snf {
namespace tf {

using namespace std::chrono;

class Test
{
protected:
	local_time *m_st;
	time_point<high_resolution_clock> m_begin;
	time_point<high_resolution_clock> m_end;
	bool m_failure;
	std::ostringstream m_strm;

	int64_t elapsed() const { return duration_cast<microseconds>(m_end - m_begin).count(); }
public:
	Test() : m_st(0), m_failure(false) {}
	virtual ~Test() { if (m_st) delete m_st; }

	virtual const char *name() const = 0;
	virtual const char *description() const = 0;
	virtual bool execute(const snf::config *) = 0;

	virtual bool run(const snf::config *config)
	{
		m_st = DBG_NEW local_time;
		m_begin = high_resolution_clock::now();
		bool passed = execute(config);
		m_end = high_resolution_clock::now();
		m_failure = !passed;
		return passed;
	}

	virtual void report()
	{
		std::cerr << name() << " - " << description() << std::endl;
		if (m_st) {
			std::cerr << "Start Time   : " << m_st->str() << std::endl;
			std::cerr << "Elapsed Time : " << elapsed() << " microseconds" << std::endl;
			std::cerr << "Status       : " << (failed() ? "FAIL" : "PASS") << std::endl;
		} else {
			std::cerr << "Skipped" << std::endl;
		}
	}

	virtual bool failed()
	{
		return m_failure;
	}
};

extern Test *TestList[];

class TestSuite : public Test
{
private:
	std::string         m_name;
	std::string         m_desc;
	local_time          *m_et;
	int                 m_failcnt;
	std::vector<Test *> m_tests;

public:
	TestSuite(const std::string &sn, const std::string &sd)
		: m_name(sn)
		, m_desc(sd)
		, m_et(0)
		, m_failcnt(0) {}

	~TestSuite()
	{
		if (m_et) delete m_et;
		for (auto t : m_tests)
			if (t)
				delete t;
		m_tests.clear();
		
	}

	void addTest(Test *test)
	{
		m_tests.push_back(test);
	}

	virtual const char *name() const
	{
		return m_name.c_str();
	}

	virtual const char *description() const
	{
		return m_desc.c_str();
	}

	virtual bool execute(const snf::config *config)
	{
		m_st = DBG_NEW local_time;

		for (auto t : m_tests) {
			if (t) {
				if (!t->run(config)) {
					m_failcnt++;
				}
				t->report();
			}
		}

		m_et = DBG_NEW local_time;
		m_failure = (m_failcnt != 0);
		return (m_failcnt == 0);
	}

	virtual void report()
	{
		std::cerr << name() << " - " << description() << std::endl;
		std::cerr << "Start Time   : " << m_st->str() << std::endl;
		std::cerr << "End Time     : " << m_st->str() << std::endl;
		std::cerr << "Failed Tests : " << m_failcnt << "/" << m_tests.size() << std::endl;
		std::cerr << "Status       : " << (failed() ? "FAIL" : "PASS") << std::endl;
	}
};

#define ASSERT_EQ(N1, N2, STR)      do {                        \
    std::cerr << STR << std::endl;                              \
    if ((N1) != (N2)) {                                         \
        std::cerr << "(" #N1 " == " #N2 ") failed at "          \
            << __FILE__ << "." << __LINE__ << std::endl;        \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_NE(N1, N2, STR)      do {                        \
    std::cerr << STR << std::endl;                              \
    if ((N1) == (N2)) {                                         \
        std::cerr << "(" #N1 " != " #N2 ") failed at "          \
            << __FILE__ << "." << __LINE__ << std::endl;        \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_EQ(S1, S2, N, STR)      do {                 \
    std::cerr << STR << std::endl;                              \
    if (memcmp((S1), (S2), (N)) != 0) {                         \
        std::cerr << "(" #S1 " == " #S2 ") failed at "          \
            << __FILE__ << "." << __LINE__ << std::endl;        \
        return false;                                           \
    }                                                           \
} while (0)

#define ASSERT_MEM_NE(S1, S2, N, STR)      do {                 \
    std::cerr << STR << std::endl;                              \
    if (memcmp((S1), (S2), (N)) == 0) {                         \
        std::cerr << "(" #S1 " != " #S2 ") failed at "          \
            << __FILE__ << "." << __LINE__ << std::endl;        \
        return false;                                           \
    }                                                           \
} while (0)

} // namespace tf
} // namespace snf

#endif // _SNF_TF_TEST_H_
