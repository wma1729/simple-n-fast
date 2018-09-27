#include "pkey.h"

class priv_key : public snf::tf::test
{
private:
	static constexpr const char *class_name = "priv_key";

	uint8_t *read_file(const std::string &fname, size_t &fsize)
	{
		snf::file f(fname, 0022);

		snf::file::open_flags flags;
		flags.o_read = true;

		f.open(flags);
		fsize = static_cast<size_t>(f.size());
		uint8_t *data = new uint8_t[fsize];

		int bread = 0;
		f.read(data, static_cast<int>(fsize), &bread);

		f.close();

		return data;
	}
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

			size_t dlen;
			uint8_t *data;

			data = read_file("test.key.der", dlen);
			snf::net::ssl::private_key pkey3(
				snf::net::ssl::ssl_data_fmt::der,
				data,
				dlen);
			delete [] data;

			ASSERT_EQ(bool, true, true, "private key 3 creation passed");
			pkey3.verify();
			ASSERT_EQ(bool, true, true, "private key 3 verification passed");

			data = read_file("test.key.pem", dlen);
			read_file("test.key.pem", dlen);
			snf::net::ssl::private_key pkey4(
				snf::net::ssl::ssl_data_fmt::pem,
				data,
				dlen,
				"Te5tP@55w0rd");
			delete [] data;

			ASSERT_EQ(bool, true, true, "private key 4 creation passed");
			pkey4.verify();
			ASSERT_EQ(bool, true, true, "private key 4 verification passed");

			EVP_PKEY *internal_key = pkey1;
			snf::net::ssl::private_key pkey5(internal_key);
			pkey5.verify();
			ASSERT_EQ(bool, true, true, "private key 5 verification passed");

			snf::net::ssl::private_key pkey6(pkey2);
			pkey6.verify();
			ASSERT_EQ(bool, true, true, "private key 6 verification passed");

			snf::net::ssl::private_key pkey7(std::move(pkey3));
			pkey7.verify();
			ASSERT_EQ(bool, true, true, "private key 7 verification passed");

			snf::net::ssl::private_key pkey8 = pkey4;
			pkey8.verify();
			ASSERT_EQ(bool, true, true, "private key 8 verification passed");

			snf::net::ssl::private_key pkey9 = std::move(pkey4);
			pkey9.verify();
			ASSERT_EQ(bool, true, true, "private key 9 verification passed");

			snf::net::finalize();
		} catch (snf::net::ssl::ssl_exception ex) {
			std::cerr << ex.what() << std::endl;
			std::vector<snf::net::ssl::ssl_error>::const_iterator I;
			for (I = ex.begin(); I != ex.end(); ++I)
				std::cerr << *I << std::endl;
			exception_caught = true;
		} catch (std::system_error ex) {
			std::cerr << "system error: " << ex.code() << std::endl;
			std::cerr << ex.what() << std::endl;
			exception_caught = true;
		} catch (std::invalid_argument ex) {
			std::cerr << "invalid argument: " << ex.what() << std::endl;
			exception_caught = true;
		}

		return !exception_caught;
	}
};
