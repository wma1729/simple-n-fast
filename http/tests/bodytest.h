#include "body.h"
#include <cstdlib>
#include <time.h>

static std::string ctable("abcdefghijklmnopqrstuvwxyz0123456789");
static size_t cindex = 0;

int
generate_body(void *buf, size_t buflen, size_t *datalen, snf::http::chunk_ext_t *)
{
	char *cbuf = static_cast<char *>(buf);
	int n = std::rand();
	n = n % 113;

	int i;
	for (i = 0; i < n; ++i)
		cbuf[i] = ctable[cindex];
	cbuf[i] = '\0';
	*datalen = n;

	cindex++;
	if (cindex == ctable.size())
		cindex = 0;

	return E_ok;
}

class bodytest : public snf::tf::test
{
private:
	static constexpr const char *class_name = "bodytest";

	bool test_generation()
	{
		snf::http::body *body = snf::http::body_factory::instance().from_functor(generate_body);
		while (body->has_next()) {
			size_t datalen = 0;
			const void *data = body->next(datalen);

			std::cout << std::hex << datalen << "\r\n";

			if (datalen) {
				const char *cdata = static_cast<const char *>(data);
				std::cout << cdata << std::endl;
			}
		}

		delete body;

		return true;
	}

public:
	bodytest() : snf::tf::test() { std::srand(static_cast<unsigned int>(time(0))); }

	~bodytest() {}

	virtual const char *name() const
	{
		return "Body Test";
	}

	virtual const char *description() const
	{
		return "Tests Body generation/parsing";
	}

	virtual bool execute(const snf::config *conf)
	{
		if (!test_generation())
			return false;

		return true;
	}
};
