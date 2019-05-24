#ifndef _SNF_NIO_H_
#define _SNF_NIO_H_

#include <string>
#include "net.h"
#include "error.h"

namespace snf {
namespace net {

/*
 * A base class (think of it as interface) that is implemented
 * by socket and ssl::connection class to provide a consistent
 * read/write interface.
 */
class nio
{
private:
	union u4 {
		float   r;      // real number
		int32_t i;      // hopefully an equal sized integer
	};

	union u8 {
		double  r;      // real number
		int64_t i;      // hopefully an equal sized integer
	};

	bool m_buffered = false;    // is read buffered?
	char *m_buf = nullptr;      // data buffer
	int  m_max = 0;             // maximum buffer size
	int  m_len = 0;             // valid data in the buffer
	int  m_idx = 0;             // next i/o index

	int read_buffered(void *, int, int *, int, int *);

public:
	nio() {}

	nio(const nio &io)
		: m_buffered(io.m_buffered)
		, m_buf(nullptr)
		, m_max(io.m_max)
		, m_len(io.m_len)
		, m_idx(io.m_idx)
	{
		if (io.m_buf) {
			m_buf = DBG_NEW char[m_max];
			memcpy(m_buf, io.m_buf, m_max);
		}
	}

	nio(nio &&io)
		: m_buffered(io.m_buffered)
		, m_max(io.m_max)
		, m_len(io.m_len)
		, m_idx(io.m_idx)
	{
		m_buf = io.m_buf;
		io.m_buf = nullptr;
	}
	
	virtual ~nio()
	{
		if (m_buf)
			delete [] m_buf;
	}

	virtual const nio & operator=(const nio &io)
	{
		if (this != &io) {
			m_buffered = io.m_buffered;

			if (m_buf) {
				delete [] m_buf;
				m_buf = nullptr;
			}

			if (io.m_buf) {
				m_buf = DBG_NEW char[m_max];
				memcpy(m_buf, io.m_buf, m_max);
			}

			m_buf = io.m_buf;
			m_max = io.m_max;
			m_len = io.m_len;
			m_idx = io.m_idx;
		}

		return *this;
	}

	virtual nio & operator=(nio &&io)
	{
		if (this != &io) {
			m_buffered = io.m_buffered;

			if (m_buf) {
				delete [] m_buf;
				m_buf = nullptr;
			}

			if (io.m_buf) {
				m_buf = io.m_buf;
				io.m_buf = nullptr;
			}

			m_max = io.m_max;
			m_len = io.m_len;
			m_idx = io.m_idx;
		}

		return *this;
	}

	virtual int readn(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;
	virtual int writen(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;

	void setbuf(int);

	int read(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int write(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0);

	int get_char(char &, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int put_char(char, int to = POLL_WAIT_FOREVER, int *oserr = 0);

	int read_string(std::string &, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int write_string(const std::string &, int to = POLL_WAIT_FOREVER, int *oserr = 0);

	/*
	 * Reads integral data of size 2, 4, or 8.
	 *
	 * @param [out] data  - integral data to read.
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
	template<typename T, typename std::enable_if<
		std::is_integral<T>::value &&
		!std::is_same<T, bool>::value, int>::type = 0>
	int read_integral(T *data, int to = POLL_WAIT_FOREVER, int *oserr = 0)
	{
		int to_read = static_cast<int>(sizeof(T));
		int bread = 0;

		if ((to_read != 2) && (to_read != 4) && (to_read != 8))
			return E_invalid_arg;

		T val;
		int retval = read(&val, to_read, &bread, to, oserr);
		if (E_ok == retval)
			if (to_read != bread)
				retval = E_read_failed;
		if (E_ok == retval)
			*data = ntoh(val);

		return retval;
	}

	/*
	 * Writes integral data of size 2, 4, or 8.
	 *
	 * @param [in]  data  - integral data to write.
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
	template<typename T, typename std::enable_if<
		std::is_integral<T>::value &&
		!std::is_same<T, bool>::value, int>::type = 0>
	int write_integral(T data, int to = POLL_WAIT_FOREVER, int *oserr = 0)
	{
		int to_write = static_cast<int>(sizeof(T));
		int bwritten = 0;

		if ((to_write != 2) && (to_write != 4) && (to_write != 8))
			return E_invalid_arg;

		data = hton(data);

		int retval = write(&data, to_write, &bwritten, to, oserr);
		if (E_ok == retval)
			if (to_write != bwritten)
				retval = E_write_failed;
		return retval;
	}

	/*
	 * Reads floating point data of size 4 or 8.
	 *
	 * @param [out] data  - floating point data to read.
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
	template<typename T, typename std::enable_if<
		std::is_floating_point<T>::value, int>::type = 0>
	int read_real(T *data, int to = POLL_WAIT_FOREVER, int *oserr = 0)
	{
		int to_read = static_cast<int>(sizeof(T));
		int bread = 0;

		if ((to_read != 4) && (to_read != 8))
			return E_invalid_arg;

		T val;
		int retval = read(&val, to_read, &bread, to, oserr);
		if (E_ok == retval)
			if (to_read != bread)
				retval = E_read_failed;
		if (E_ok == retval) {
			if (to_read == 4) {
				u4 f;
				f.i = ntoh(val);
				*data = f.r;
			} else if (to_read == 8) {
				u8 d;
				d.i = ntoh(val);
				*data = d.r;
			}
		}

		return retval;
	}

	/*
	 * Writes floating point data of size 4 or 8.
	 *
	 * @param [out] data  - floating point data to write.
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
	template<typename T, typename std::enable_if<
		std::is_floating_point<T>::value, int>::type = 0>
	int write_real(T data, int to = POLL_WAIT_FOREVER, int *oserr = 0)
	{
		int to_write = static_cast<int>(sizeof(T));
		int bwritten = 0;

		if (to_write == 4) {
			u4 f;
			f.r = data;
			data = hton(f.i);
		} else if (to_write == 8) {
			u8 d;
			d.r = data;
			data =  hton(d.i);
		} else {
			return E_invalid_arg;
		}

		int retval = write(&data, to_write, &bwritten, to, oserr);
		if (E_ok == retval)
			if (to_write != bwritten)
				retval = E_write_failed;
		return retval;
	}

	int readline(std::string &, int to = POLL_WAIT_FOREVER, int *oserr = 0);
};

} // namespace net
} // namespace snf

#endif // _SNF_NIO_H_
