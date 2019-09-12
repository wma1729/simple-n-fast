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

bool verbose = false;

class assertion_failure
{
private:
	std::string m_msg;
	std::string m_file;
	int         m_line;

public:
	assertion_failure(const std::string &msg, const char *file, int line)
		: m_msg(msg)
		, m_file(file)
		, m_line(line)
	{
	}

	assertion_failure(const char *msg, const char *file, int line)
		: m_msg(msg)
		, m_file(file)
		, m_line(line)
	{
	}

	const char *what()
	{
		std::ostringstream oss;
		oss << " at " << m_file << "." << m_line;
		m_msg += oss.str();
		return m_msg.c_str();
	}
};

class test
{
protected:
	local_time *m_st;
	time_point<high_resolution_clock> m_begin;
	time_point<high_resolution_clock> m_end;
	bool m_failure;
	std::ostringstream m_strm;

	int64_t elapsed() const { return duration_cast<microseconds>(m_end - m_begin).count(); }
public:
	test() : m_st(0), m_failure(false) {}
	virtual ~test() { delete m_st; }

	virtual const char *name() const = 0;
	virtual const char *description() const = 0;
	virtual bool execute(const snf::config *) = 0;

	virtual bool run(const snf::config *config)
	{
		m_st = DBG_NEW local_time;
		m_begin = high_resolution_clock::now();

		bool passed = false;
		try {
			passed = execute(config);
		} catch (assertion_failure &ex) {
			std::cerr << ex.what() << std::endl;
		}

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

extern test *test_list[];

class test_suite : public test
{
private:
	std::string         m_name;
	std::string         m_desc;
	local_time          *m_et;
	int                 m_failcnt;
	std::vector<test *> m_tests;

public:
	test_suite(const std::string &sn, const std::string &sd)
		: m_name(sn)
		, m_desc(sd)
		, m_et(0)
		, m_failcnt(0) {}

	~test_suite()
	{
		for (auto t : m_tests) {
			delete t;
		}
		m_tests.clear();
		delete m_et;
	}

	void add(test *test)
	{
		if (test)
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
		std::cerr << "End Time     : " << m_et->str() << std::endl;
		std::cerr << "Failed Tests : " << m_failcnt << "/" << m_tests.size() << std::endl;
		std::cerr << "Status       : " << (failed() ? "FAIL" : "PASS") << std::endl;
	}
};

class assertion
{
public:
	template<typename T>
	static void eq(
		T lhs, const char *lhs_expr,
		T rhs, const char *rhs_expr,
		const std::string &msg,
		const char *file, int line)
	{
		if (lhs != rhs) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< "=="
				<< rhs_expr
				<< ") failed; lhs = "
				<< lhs
				<< ", rhs = "
				<< rhs;
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}

	template<typename T>
	static void ne(
		T lhs, const char *lhs_expr,
		T rhs, const char *rhs_expr,
		const std::string &msg,
		const char *file, int line)
	{
		if ((lhs == rhs)) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< "!="
				<< rhs_expr
				<< ") failed; lhs = "
				<< lhs
				<< ", rhs = "
				<< rhs;
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}

	template<typename T>
	static void gt(
		T lhs, const char *lhs_expr,
		T rhs, const char *rhs_expr,
		const std::string &msg,
		const char *file, int line)
	{
		if (lhs <= rhs) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< ">"
				<< rhs_expr
				<< ") failed; lhs = "
				<< lhs
				<< ", rhs = "
				<< rhs;
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}

	template<typename T>
	static void ge(
		T lhs, const char *lhs_expr,
		T rhs, const char *rhs_expr,
		const std::string &msg,
		const char *file, int line)
	{
		if (lhs < rhs) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< ">="
				<< rhs_expr
				<< ") failed; lhs = "
				<< lhs
				<< ", rhs = "
				<< rhs;
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}

	static void mem_eq(
		const void *lhs, const char *lhs_expr,
		const void *rhs, const char *rhs_expr,
		size_t n, const std::string &msg,
		const char *file, int line)
	{
		if (memcmp(lhs, rhs, n) != 0) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< "!="
				<< rhs_expr
				<< ") failed";
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}

	static void mem_ne(
		const void *lhs, const char *lhs_expr,
		const void *rhs, const char *rhs_expr,
		size_t n, const std::string &msg,
		const char *file, int line)
	{
		if (memcmp(lhs, rhs, n) == 0) {
			std::cerr << msg << " ... OUCH" << std::endl;

			std::ostringstream oss;
			oss << "Assertion ("
				<< lhs_expr
				<< "=="
				<< rhs_expr
				<< ") failed";
			throw assertion_failure(oss.str(), file, line);
		}

		if (verbose)
			std::cerr << msg << " ... OK" << std::endl;
	}
};

#define ASSERT_EQ(T, N1, N2, STR)       \
	snf::tf::assertion::eq<T>(N1, #N1, N2, #N2, STR, __FILE__, __LINE__)
#define ASSERT_NE(T, N1, N2, STR)       \
	snf::tf::assertion::ne<T>(N1, #N1, N2, #N2, STR, __FILE__, __LINE__)
#define ASSERT_GT(T, N1, N2, STR)       \
	snf::tf::assertion::gt<T>(N1, #N1, N2, #N2, STR, __FILE__, __LINE__)
#define ASSERT_GE(T, N1, N2, STR)       \
	snf::tf::assertion::ge<T>(N1, #N1, N2, #N2, STR, __FILE__, __LINE__)
#define ASSERT_MEM_EQ(S1, S2, N, STR)   \
	snf::tf::assertion::mem_eq(S1, #S1, S2, #S2, N, STR, __FILE__, __LINE__)
#define ASSERT_MEM_NE(S1, S2, N, STR)   \
	snf::tf::assertion::mem_ne(S1, #S1, S2, #S2, N, STR, __FILE__, __LINE__)

#define TEST_PASS(MSG)  do { std::cerr << MSG << " ... OK" << std::endl; } while (0)
#define TEST_FAIL(MSG)  throw snf::tf::assertion_failure(MSG, __FILE__, __LINE__)

} // namespace tf
} // namespace snf

#endif // _SNF_TF_TEST_H_
