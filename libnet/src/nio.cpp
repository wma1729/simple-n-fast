#include "nio.h"
#include "dbg.h"
#include <memory>
#include <algorithm>

namespace snf {
namespace net {

/*
 * Enable and sets the read buffer size. Call it once. The subsequent calls are noops.
 *
 * @param [in] bufsize - buffer size. 64 <= bufsize << 65536.
 */
void
nio::setbuf(int bufsize)
{
	if (m_buffered)
		return;

	m_buffered = true;

	if (bufsize <  64)
		bufsize = 64;
	else if (bufsize > 65536)
		bufsize = 65536;

	m_buf = DBG_NEW char[bufsize];
	m_max = bufsize;
	m_len = 0;
	m_idx = 0;
}

/*
 * Reads buffered data.
 */
int
nio::read_buffered(void *buf, int to_read, int *bread, int to, int *oserr)
{
	int     retval = E_ok;
	int     n = 0, nbytes = 0;
	char    *cbuf = static_cast<char *>(buf);

	if (buf == nullptr)
		return E_invalid_arg;

	if (to_read <= 0)
		return E_invalid_arg;

	if (bread == nullptr)
		return E_invalid_arg;

	while (to_read) {
		if (m_idx < m_len) {
			// there is data available in buffer
			n = std::min((m_len - m_idx), to_read);
			memcpy(cbuf, m_buf + m_idx, n);
			m_idx += n;
			to_read -= n;
			nbytes += n;
		}

		if (to_read) {
			if (m_idx >= m_len) {
				retval = E_invalid_state;
			} else {
				m_idx = m_len = 0;
				retval = read(m_buf, m_max, &m_len, to, oserr);
			}

			if ((retval != E_ok) || (m_len == 0))
				break;
		}
	}

	*bread = nbytes;

	return retval;
}

int
nio::read(void *buf, int to_read, int *bread, int to, int *oserr)
{
	if (!m_buffered)
		return readn(buf, to_read, bread, to, oserr);
	return read_buffered(buf, to_read, bread, to, oserr);
}

int
nio::write(const void *buf, int to_write, int *bwritten, int to, int *oserr)
{
	return writen(buf, to_write, bwritten, to, oserr);
}

/*
 * Reads a single character.
 *
 * @param [out] c     - character to read.
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
nio::get_char(char &c, int to, int *oserr)
{
	int bread = 0;
	char buf[1];

	int retval = read(buf, 1, &bread, to, oserr);
	if (E_ok == retval)
		if (1 != bread)
			retval = E_read_failed;

	if (E_ok == retval)
		c = buf[0];

	return retval;
}

/*
 * Writes a singe character.
 *
 * @param [out] c     - character to write.
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
nio::put_char(char c, int to, int *oserr)
{
	int bwritten = 0;
	char buf[1] = { c };

	int retval = write(buf, 1, &bwritten, to, oserr);
	if (E_ok == retval)
		if (1 != bwritten)
			retval = E_write_failed;

	return retval;
}

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

/*
 * Reads a line terminated by newline ('\n'). This will continue
 * readline until a new line is encountered.
 *
 * @param [out] line  - line read.
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
nio::readline(std::string &line, int to, int *oserr)
{
	int     retval = E_ok;
	int     n;
	char    buf[1];

	do {
		n = 0;
		retval = read(buf, 1, &n, to, oserr);
		if (retval != E_ok) {
			break;
		} else if (n == 0) {
			retval = E_read_failed;
			break;
		}

		if (n == 1)
			line.push_back(buf[0]);
	} while (buf[0] != '\n');

	return retval;
}

} // namespace net
} // namespace snf
