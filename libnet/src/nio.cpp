#include "nio.h"
#include "dbg.h"
#include <memory>

namespace snf {
namespace net {

int
nio::read_string(std::string &str, int to, int *oserr)
{
	int retval;
	int to_read;

	retval = read_integral(&to_read, to, oserr);
	if (E_ok == retval) {
		int bread = 0;
		std::unique_ptr<char []> ptr(DBG_NEW char[to_read]);

		retval = read(ptr.get(), to_read, &bread, to, oserr);
		if (E_ok == retval)
			if (to_read != bread)
				retval = E_read_failed;

		if (E_ok == retval)
			str.insert(0, ptr.get(), bread);
	}

	return retval;
}

int
nio::write_string(const std::string &str, int to, int *oserr)
{
	int retval;
	int to_write = static_cast<int>(str.size());

	retval = write_integral(to_write, to, oserr);
	if (E_ok == retval) {
		int bwritten = 0;

		retval = write(str.c_str(), to_write, &bwritten, to, oserr);
		if (E_ok == retval)
			if (to_write != bwritten)
				retval = E_write_failed;
	}

	return retval;
}

} // namespace net
} // namespace snf
