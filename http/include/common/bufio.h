#ifndef _SNF_BUFIO_H_
#define _SNF_BUFIO_H_

#include "nio.h"

namespace snf {
namespace http {

class buffered_reader
{
private:
	struct buf
	{
		char *m_data;  // data buffer
		int  m_max;    // maximum buffer size
		int  m_len;    // valid data in the buffer
		int  m_idx;    // next io index

		buf(int size)
		{
			m_data = new char[size];
			m_max = size;
			reset();
		}

		~buf()
		{
			delete [] m_data;
		}

		void reset() { m_len = m_idx = 0; }
		bool empty() const { return (m_len == m_idx); }
		int available() const { return (m_len - m_idx); }

		const char *data(int n)
		{
			if (n > available())
				return nullptr;

			char *ptr = m_data + m_idx;
			m_idx += n;
			return ptr;
		}
	};

	snf::net::nio   &m_io;
	buf             m_buf;

public:
	buffered_reader(snf::net::nio &io, int size = 8192)
		: m_io(io) , m_buf(size) {}
	~buffered_reader() {}

	int read(void *, int, int *, int to = snf::net::POLL_WAIT_FOREVER, int *oserr = 0);
	int readline(std::string &, int to = snf::net::POLL_WAIT_FOREVER, int *oserr = 0);
};

} // namespace http
} // namespace snf

#endif // _SNF_BUFIO_H_
