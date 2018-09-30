#include "crt.h"

class certificate : public snf::tf::test
{
private:
	static constexpr const char *class_name = "certificate";

public:
	certificate() : snf::tf::test() {}
	~certificate() {}

	virtual const char *name() const
	{
		return "Certificate";
	}

	virtual const char *description() const
	{
		return "Tests X509 certificates";
	}

	virtual bool execute(const snf::config *conf)
	{
		bool exception_caught = false;

		try {
			snf::net::initialize(true);

			snf::net::ssl::x509_certificate cert1(
				snf::net::ssl::ssl_data_fmt::der,
				"unittest.simplenfast.org/unittest.simplenfast.org.cert.der");
			ASSERT_EQ(bool, true, true, "certificate 1 creation passed");

			snf::net::ssl::x509_certificate cert2(
				snf::net::ssl::ssl_data_fmt::pem,
				"unittest.simplenfast.org/unittest.simplenfast.org.cert.pem",
				"P@ssw0rd");
			ASSERT_EQ(bool, true, true, "certificate 2 creation passed");

			uint8_t *data = nullptr;
			size_t dlen = 0;

			snf::read_file(
				"unittest.simplenfast.org/unittest.simplenfast.org.cert.der",
				data,
				&dlen);
			snf::net::ssl::x509_certificate cert3(
				snf::net::ssl::ssl_data_fmt::der,
				data,
				dlen);
			delete [] data;
			data = nullptr;
			dlen = 0;

			ASSERT_EQ(bool, true, true, "certificate 3 creation passed");

			snf::read_file(
				"unittest.simplenfast.org/unittest.simplenfast.org.cert.pem",
				data,
				&dlen);
			snf::net::ssl::x509_certificate cert4(
				snf::net::ssl::ssl_data_fmt::pem,
				data,
				dlen,
				"P@ssw0rd");
			delete [] data;
			data = nullptr;
			dlen = 0;

			ASSERT_EQ(bool, true, true, "certificate 4 creation passed");

			X509 *internal_crt = cert1;
			snf::net::ssl::x509_certificate cert5(internal_crt);
			ASSERT_EQ(bool, true, true, "certificate 5 creation passed");

			snf::net::ssl::x509_certificate cert6(cert2);
			ASSERT_EQ(bool, true, true, "certificate 6 creation passed");

			snf::net::ssl::x509_certificate cert7(std::move(cert3));
			ASSERT_EQ(bool, true, true, "certificate 7 creation passed");

			snf::net::ssl::x509_certificate cert8 = cert4;
			ASSERT_EQ(bool, true, true, "certificate 8 creation passed");

			snf::net::ssl::x509_certificate cert9 = std::move(cert4);
			ASSERT_EQ(bool, true, true, "certificate 9 creation passed");

			std::string cn = "unittest.simplenfast.org";
			ASSERT_EQ(const std::string &, cn, cert9.common_name(), "common name matches");
			std::string sub = "C = US, ST = Minnesota, L = Mahtomedi, O = www.simplenfast.org, OU = Department of Perennial Learning, CN = unittest.simplenfast.org";
			ASSERT_EQ(const std::string &, sub, cert9.subject(), "subject matches");

			std::string iss = "C = US, ST = Minnesota, O = www.simplenfast.org, OU = Department of Perennial Learning, CN = IntermediateCA";
			ASSERT_EQ(const std::string &, iss, cert9.issuer(), "issuer matches");

			const std::vector<snf::net::ssl::alternate_name> &altnames =
				cert9.alternate_names();
			ASSERT_EQ(size_t, altnames.size(), 3, "alternate names size matched");

			std::vector<snf::net::ssl::alternate_name>::const_iterator I;
			for (I = altnames.begin(); I != altnames.end(); ++I) {
				if (I->type == "DNS")
					ASSERT_EQ(const std::string &, I->name, "localhost", "DNS name matches");
				else if (I->type == "IP")
					ASSERT_EQ(const std::string &, I->name, "127.0.0.1", "IP address matches");
				else if (I->type == "URI")
					ASSERT_EQ(const std::string &, I->name, "http://127.0.0.1/", "URI matches");
			}

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
		} catch (std::runtime_error ex) {
			std::cerr << "runtime error: " << ex.what() << std::endl;
			exception_caught = true;
		}

		return !exception_caught;
	}
};
