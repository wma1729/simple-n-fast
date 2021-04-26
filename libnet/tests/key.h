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
		bool exception_caught = false;

		try {
			snf::net::initialize();

			snf::ssl::pkey pkey1(
				snf::ssl::data_fmt::der,
				"unittest.simplenfast.org/unittest.simplenfast.org.key.der");
			ASSERT_EQ(bool, true, true, "private key 1 creation passed");
			pkey1.verify();
			ASSERT_EQ(bool, true, true, "private key 1 verification passed");

			snf::ssl::pkey pkey2(
				snf::ssl::data_fmt::pem,
				"unittest.simplenfast.org/unittest.simplenfast.org.key.pem",
				"Te5tP@55w0rd");
			ASSERT_EQ(bool, true, true, "private key 2 creation passed");
			pkey2.verify();
			ASSERT_EQ(bool, true, true, "private key 2 verification passed");

			uint8_t *data = nullptr;
			size_t dlen = 0;

			snf::read_file(
				"unittest.simplenfast.org/unittest.simplenfast.org.key.der",
				data,
				&dlen);
			snf::ssl::pkey pkey3(
				snf::ssl::data_fmt::der,
				data,
				dlen);
			delete [] data;
			data = nullptr;
			dlen = 0;

			ASSERT_EQ(bool, true, true, "private key 3 creation passed");
			pkey3.verify();
			ASSERT_EQ(bool, true, true, "private key 3 verification passed");

			snf::read_file(
				"unittest.simplenfast.org/unittest.simplenfast.org.key.pem",
				data,
				&dlen);
			snf::ssl::pkey pkey4(
				snf::ssl::data_fmt::pem,
				data,
				dlen,
				"Te5tP@55w0rd");
			delete [] data;
			data = nullptr;
			dlen = 0;

			ASSERT_EQ(bool, true, true, "private key 4 creation passed");
			pkey4.verify();
			ASSERT_EQ(bool, true, true, "private key 4 verification passed");

			EVP_PKEY *internal_key = pkey1;
			snf::ssl::pkey pkey5(internal_key);
			pkey5.verify();
			ASSERT_EQ(bool, true, true, "private key 5 verification passed");

			snf::ssl::pkey pkey6(pkey2);
			pkey6.verify();
			ASSERT_EQ(bool, true, true, "private key 6 verification passed");

			snf::ssl::pkey pkey7(std::move(pkey3));
			pkey7.verify();
			ASSERT_EQ(bool, true, true, "private key 7 verification passed");

			snf::ssl::pkey pkey8 = pkey4;
			pkey8.verify();
			ASSERT_EQ(bool, true, true, "private key 8 verification passed");

			snf::ssl::pkey pkey9 = std::move(pkey4);
			pkey9.verify();
			ASSERT_EQ(bool, true, true, "private key 9 verification passed");

		} catch (const snf::ssl::exception &ex) {
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
