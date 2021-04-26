#include "keymgr.h"
#include "random.h"
#include <time.h>
#include <string.h>

namespace snf {
namespace ssl {

/*
 * Gets a key record. A new key is created if:
 * - one does not exist.
 * - the key has expired.
 *
 * The lifetime of the key can be specified at the time
 * of key manager creation. The default is 1 hour. The
 * expired key is maintained for 80% of the lifetime i.e.
 * 48 minutes by default.
 *
 * @return key record or nullptr if the key could not be
 *         created.
 *
 * @throws snf::ssl::exception if the key could not
 *         be created.
 */
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
		safestr ss(KEY_SIZE + AES_SIZE + HMAC_SIZE);
		random r;
		r.bytes(ss, true);
		m_cur = DBG_NEW keyrec;
		uint8_t *ptr = ss.data();
		memcpy(m_cur->key_name, ptr, KEY_SIZE); ptr += KEY_SIZE;
		memcpy(m_cur->aes_key, ptr, AES_SIZE); ptr += AES_SIZE;
		memcpy(m_cur->hmac_key, ptr, HMAC_SIZE);
		m_cur->expire = now + m_life;
	}

	return m_cur;
}

/*
 * Finds the key record using the given key name.
 *
 * @param [in] name - key name.
 * @param [in] len  - key name length.
 *
 * @return key record.
 */
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
} // namespace snf
