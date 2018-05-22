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
	virtual ~test() { if (m_st) delete m_st; }

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
		if (m_et) delete m_et;
		for (auto t : m_tests)
			if (t)
				delete t;
		m_tests.clear();
		
	}

	void add(test *test)
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
		std::cerr << "End Time     : " << m_et->str() << std::endl;
		std::cerr << "Failed Tests : " << m_failcnt << "/" << m_tests.size() << std::endl;
		std::cerr << "Status       : " << (failed() ? "FAIL" : "PASS") << std::endl;
	}
};

template<typename Target, typename Source>
Target narrow_cast(Source v)
{
	auto r = static_cast<Target>(v);
	if (static_cast<Source>(r) != v) {
		throw std::runtime_error("narrowing the value causes data loss");
	}
	return r;
}

class assertion
{
public:
	static void eq(uint64_t lhs, uint64_t rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs != rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "!="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void eq(int64_t lhs, int64_t rhs, const std::string &msg,
		const char *file, int line)
	{
		eq(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void eq(uint32_t lhs, uint32_t rhs, const std::string &msg,
		const char *file, int line)
	{
		eq(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void eq(int lhs, int rhs, const std::string &msg,
		const char *file, int line)
	{
		eq(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void eq(double lhs, double rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs != rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "!="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void eq(float lhs, float rhs, const std::string &msg,
		const char *file, int line)
	{
		eq(narrow_cast<double>(lhs), narrow_cast<double>(rhs), msg, file, line);
	}

	static void eq(const void *lhs, const void *rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs != rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "!="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void ne(uint64_t lhs, uint64_t rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs == rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "=="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void ne(int64_t lhs, int64_t rhs, const std::string &msg,
		const char *file, int line)
	{
		ne(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void ne(uint32_t lhs, uint32_t rhs, const std::string &msg,
		const char *file, int line)
	{
		ne(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void ne(int lhs, int rhs, const std::string &msg,
		const char *file, int line)
	{
		ne(static_cast<uint64_t>(lhs), static_cast<uint64_t>(rhs), msg, file, line);
	}

	static void ne(double lhs, double rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs == rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "=="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void ne(float lhs, float rhs, const std::string &msg,
		const char *file, int line)
	{
		ne(narrow_cast<double>(lhs), narrow_cast<double>(rhs), msg, file, line);
	}

	static void ne(const void *lhs, const void *rhs, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (lhs == rhs)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "=="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void mem_eq(const void *lhs, const void *rhs, size_t n, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (memcmp(lhs, rhs, n) != 0)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "!="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}

	static void mem_ne(const void *lhs, const void *rhs, size_t n, const std::string &msg,
		const char *file, int line)
	{
		if (verbose || (memcmp(lhs, rhs, n) == 0)) {
			std::cerr << msg << std::endl;

			if (!verbose) {
				std::ostringstream oss;
				oss << "Assertion ("
					<< lhs
					<< "=="
					<< rhs
					<< ") failed";
				throw assertion_failure(oss.str(), file, line);
			}
		}
	}
};

#define ASSERT_EQ(N1, N2, STR)        snf::tf::assertion::eq(N1, N2, STR, __FILE__, __LINE__)
#define ASSERT_NE(N1, N2, STR)        snf::tf::assertion::ne(N1, N2, STR, __FILE__, __LINE__)
#define ASSERT_MEM_EQ(S1, S2, N, STR) snf::tf::assertion::mem_eq(S1, S2, N, STR, __FILE__, __LINE__)
#define ASSERT_MEM_NE(S1, S2, N, STR) snf::tf::assertion::mem_ne(S1, S2, N, STR, __FILE__, __LINE__)

} // namespace tf
} // namespace snf

#endif // _SNF_TF_TEST_H_
