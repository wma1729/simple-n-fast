#include "thrdpool.h"

static int
multiply_return_value(int a, int b)
{
	return a * b;
}

static void
multiply_out_param(int &c, int a, int b)
{
	c = a * b;
}

struct Multiply
{
private:
	int a;
	int b;

public:
	Multiply(int aa, int bb)
		: a(aa)
		, b(bb)
	{
	}

	int operator()()
	{
		return a * b;
	}
};

class tp : public snf::tf::test
{
private:
	static constexpr const char *class_name = "tp";

public:
	tp() : snf::tf::test() {}
	~tp() {}

	virtual const char *name() const
	{
		return "Thread pool";
	}

	virtual const char *description() const
	{
		return "Tests thread pool";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::thread_pool pool;
		std::cout << "thread count = " << pool.thread_count() << std::endl;
		std::cout << "request count = " << pool.request_count() << std::endl;
		std::cout << "threads waiting = " << pool.threads_waiting() << std::endl;

		auto f1 = pool.submit(multiply_return_value, 7, 9);
		ASSERT_EQ(int, f1.get(), 63, "product of 7 and 9 is 63");

		int i = 0;
		auto f2 = pool.submit(multiply_out_param, std::ref(i), 6, 8);
		f2.get();
		ASSERT_EQ(int, i, 48, "product of 6 and 8 is 48");

		Multiply multiply_8x9(8, 9);
		auto f3 = pool.submit(std::ref(multiply_8x9));
		ASSERT_EQ(int, f3.get(), 72, "product of 8 and 9 is 72");

		Multiply multiply_9x9(9, 9);
		auto f4 = pool.submit(std::move(multiply_9x9));
		ASSERT_EQ(int, f4.get(), 81, "product of 9 and 9 is 81");

		auto f5 = pool.submit([] { return 11 * 11; });
		ASSERT_EQ(int, f5.get(), 121, "product of 11 and 11 is 121");

		return true;
	}
};
