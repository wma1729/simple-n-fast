#include "nio.h"
#include "dbg.h"
#include <memory>

namespace snf {
namespace net {

/*
 * Reads string. String is read as <4-byte-string-length><string>.
 *
 * @param [out] str   - string to read.
 * @param [in]  to    - timeout in milliseconds.
 *                      POLL_WAIT_FOREVER for inifinite wait.
 *                      POLL_WAIT_NONE for no wait.
 * @param [out] oserr - system error in case of failure, if not null.
 *
 * @return E_ok on success, -ve error code on failure.
 *
 * The call can possibly throw one or more exceptions if the derived
 * class of implementation of read throws one.
 */
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

/*
 * Writes string. String is written as <4-byte-string-length><string>.
 *
 * @param [out] str   - string to write.
 * @param [in]  to    - timeout in milliseconds.
 *                      POLL_WAIT_FOREVER for inifinite wait.
 *                      POLL_WAIT_NONE for no wait.
 * @param [out] oserr - system error in case of failure, if not null.
 *
 * @return E_ok on success, -ve error code on failure.
 *
 * The call can possibly throw one or more exceptions if the derived
 * class of implementation of write throws one.
 */
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
