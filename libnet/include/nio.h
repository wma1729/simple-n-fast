#ifndef _SNF_NIO_H_
#define _SNF_NIO_H_

#include <string>
#include "net.h"
#include "error.h"

namespace snf {
namespace net {

class nio
{
public:
	virtual int read(void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;
	virtual int write(const void *, int, int *, int to = POLL_WAIT_FOREVER, int *oserr = 0) = 0;

	int read_string(std::string &, int to = POLL_WAIT_FOREVER, int *oserr = 0);
	int write_string(const std::string &, int to = POLL_WAIT_FOREVER, int *oserr = 0);

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
};

} // namespace net
} // namespace snf

#endif // _SNF_NIO_H_
