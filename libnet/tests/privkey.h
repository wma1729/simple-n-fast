#include "pkey.h"

class priv_key : public snf::tf::test
{
private:
	static constexpr const char *class_name = "priv_key";

public:
	priv_key() : snf::tf::test() {}
	~priv_key() {}

	virtual const char *name() const
	{
		return "PrivateKey";
	}

	virtual const char *description() const
	{
		return "Tests private key";
	}

	virtual bool execute(const snf::config *conf)
	{
		try {
			snf::net::initialize(true);
			snf::net::ssl::private_key pkey1(
				snf::net::ssl::ssl_data_fmt::der,
				"test.key.der");
			ASSERT_EQ(bool, true, true, "private key 1 creation passed");
			pkey1.verify();
			ASSERT_EQ(bool, true, true, "private key 1 verification passed");

			snf::net::ssl::private_key pkey2(
				snf::net::ssl::ssl_data_fmt::pem,
				"test.key.pem",
				"Te5tP@55w0rd");
			ASSERT_EQ(bool, true, true, "private key 2 creation passed");
			pkey2.verify();
			ASSERT_EQ(bool, true, true, "private key 2 verification passed");
		} catch (std::invalid_argument ex) {
			std::cerr << "invalid argument: " << ex.what() << std::endl;
		} catch (std::system_error ex) {
			std::cerr << "system error: " << ex.code() << std::endl;
			std::cerr << ex.what() << std::endl;
		} catch (snf::net::ssl::ssl_exception ex) {
			std::cerr << ex.what() << std::endl;
			std::vector<snf::net::ssl::ssl_error>::const_iterator I;
			for (I = ex.begin(); I != ex.end(); ++I)
				std::cerr << *I << std::endl;
		}

		return true;
	}
};
