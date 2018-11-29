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

public:
	virtual int read(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;
	virtual int write(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;

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
};

} // namespace net
} // namespace snf

#endif // _SNF_NIO_H_
