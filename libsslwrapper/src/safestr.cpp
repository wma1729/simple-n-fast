#include "safestr.h"

namespace snf {
namespace ssl {


safestr::safestr(size_t len)
	: m_len(len)
{
	memset(m_bytes, 0, sizeof(m_bytes));

	if (use_stack_data()) {
		m_data = m_bytes;
	} else {
		m_data = DBG_NEW uint8_t[m_len];
		memset(m_data, 0, m_len);
	}
}

safestr::safestr(const safestr &ss)
{
	m_len = ss.m_len;
	memset(m_bytes, 0, sizeof(m_bytes));

	if (use_stack_data()) {
		memcpy(m_bytes, ss.m_bytes, m_len);
		m_data = m_bytes;
	} else {
		m_data = DBG_NEW uint8_t[m_len];
		memcpy(m_data, ss.m_data, m_len);
	}
}

safestr::safestr(safestr &&ss)
{
	m_len = ss.m_len;
	memset(m_bytes, 0, sizeof(m_bytes));

	if (use_stack_data()) {
		memcpy(m_bytes, ss.m_bytes, m_len);
		m_data = m_bytes;
	} else {
		m_data = ss.m_data;
	}

	memset(ss.m_bytes, 0, sizeof(ss.m_bytes));
	ss.m_data = nullptr;
}

safestr &
safestr::operator=(const safestr &ss)
{
	if (this != &ss) {
		cleanup();

		m_len = ss.m_len;

		if (use_stack_data()) {
			memcpy(m_bytes, ss.m_bytes, m_len);
			m_data = m_bytes;
		} else {
			m_data = DBG_NEW uint8_t[m_len];
			memcpy(m_data, ss.m_data, m_len);
		}
	}
	return *this;
}

safestr &
safestr::operator=(safestr &&ss)
{
	if (this != &ss) {
		cleanup();

		m_len = ss.m_len;

		if (use_stack_data()) {
			memcpy(m_bytes, ss.m_bytes, m_len);
			m_data = m_bytes;
		} else {
			m_data = ss.m_data;
		}

		memset(ss.m_bytes, 0, sizeof(ss.m_bytes));
		ss.m_data = nullptr;
	}
	return *this;
}

} // namespace ssl
} // namespace snf
