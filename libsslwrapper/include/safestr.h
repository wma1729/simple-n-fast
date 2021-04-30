#ifndef _SNF_SSL_H_
#define _SNF_SSL_H_

#include "common.h"

namespace snf {
namespace ssl {

class safestr
{
private:
	static const int STACK_BYTES = 64;

	uint8_t	    m_bytes[STACK_BYTES];
	uint8_t     *m_data = nullptr;
	size_t      m_len = 0;

	bool use_stack_data() const
	{
		return (m_len <= STACK_BYTES);
	}

	void cleanup()
	{
		if (use_stack_data()) {
			memset(m_bytes, 0, sizeof(m_bytes));
		} else {
			memset(m_data, 0, m_len);
			delete [] m_data;
		}
		m_data = nullptr;
	}

public:
	safestr(size_t);
	safestr(const safestr &);
	safestr(safestr &&);
	~safestr() { cleanup(); }

	safestr & operator=(const safestr &);
	safestr & operator=(safestr &&);

	uint8_t *data() const { return m_data; }
	size_t length() const { return m_len; }
};

} // namespace ssl
} // namespace snf

#endif // _SNF_SSL_H_
