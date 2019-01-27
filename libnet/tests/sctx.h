#include "ctx.h"

class sctx : public snf::tf::test
{
private:
	static constexpr const char *class_name = "sctx";

public:
	sctx() : snf::tf::test() {}
	~sctx() {}

	virtual const char *name() const
	{
		return "Certificate";
	}

	virtual const char *description() const
	{
		return "Tests SSL context";
	}

	virtual bool execute(const snf::config *conf)
	{
		bool exception_caught = false;

		try {
			snf::net::initialize(true);
			snf::net::ssl::context ctx1;
			ASSERT_EQ(bool, true, true, "ssl context creation passed");
		} catch (const snf::net::ssl::exception &ex) {
			std::cerr << ex.what() << std::endl;
			for (auto I = ex.begin(); I != ex.end(); ++I)
				std::cerr << *I << std::endl;
			exception_caught = true;
		} catch (const std::system_error &ex) {
			std::cerr << "system error: " << ex.code() << std::endl;
			std::cerr << ex.what() << std::endl;
			exception_caught = true;
		} catch (const std::invalid_argument &ex) {
			std::cerr << "invalid argument: " << ex.what() << std::endl;
			exception_caught = true;
		} catch (const std::runtime_error &ex) {
			std::cerr << "runtime error: " << ex.what() << std::endl;
			exception_caught = true;
		}

		return !exception_caught;
	}
};
