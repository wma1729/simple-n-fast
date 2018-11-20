#include "keymgr.h"
#include "dbg.h"
#include <time.h>

namespace snf {
namespace net {
namespace ssl {

const keyrec *
basic_keymgr::get()
{
	std::lock_guard<std::mutex> guard(m_lock);

	time_t now = time(0);

	if (m_cur && (m_cur->expire < now)) {
		if (m_old) delete m_old;
		m_old = m_cur;
		m_old->expire = now + static_cast<time_t>(m_life * 0.8);
		m_cur = nullptr;
	}

	if (!m_cur) {
		uint8_t buf[KEY_SIZE + AES_SIZE + HMAC_SIZE];
		if (ssl_library::instance().rand_bytes()
			(buf, KEY_SIZE + AES_SIZE + HMAC_SIZE) != 1)
			throw ssl_exception("failed to generate random data");

		m_cur = DBG_NEW keyrec;
		uint8_t *ptr = buf;
		memcpy(m_cur->key_name, ptr, KEY_SIZE); ptr += KEY_SIZE;
		memcpy(m_cur->aes_key, ptr, AES_SIZE); ptr += AES_SIZE;
		memcpy(m_cur->hmac_key, ptr, HMAC_SIZE);
		m_cur->expire = now + m_life;
	}

	return m_cur;
}

const keyrec *
basic_keymgr::find(const uint8_t *name, size_t len)
{
	std::lock_guard<std::mutex> guard(m_lock);

	if (len != KEY_SIZE)
		return nullptr;

	if (m_cur)
		if (memcmp(m_cur->key_name, name, len) == 0)
			return m_cur;

	if (m_old)
		if (memcmp(m_old->key_name, name, len) == 0) 
			return m_old;

	return nullptr;
}

} // namespace ssl
} // namespace net
} // namespace snf
