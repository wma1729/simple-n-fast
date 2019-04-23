#include "bufio.h"
#include <algorithm>

namespace snf {
namespace http {

int
buffered_reader::read(void *buf, int to_read, int *bread, int to, int *oserr)
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
		if (!m_buf.empty()) {
			n = std::min(m_buf.available(), to_read);
			memcpy(cbuf, m_buf.data(n), n);
			to_read -= n;
			nbytes += n;
		}

		if (to_read) {
			if (!m_buf.empty()) {
				retval = E_invalid_state;
			} else {
				m_buf.reset();
				retval = m_io.read(m_buf.m_data, m_buf.m_max, &m_buf.m_len, to, oserr);
			}

			if ((retval != E_ok) || (m_buf.m_len == 0))
				break;
		}
	}

	*bread = nbytes;

	return retval;
}

int
buffered_reader::readline(std::string &line, int to, int *oserr)
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

} // namespace http
} // namespace snf
