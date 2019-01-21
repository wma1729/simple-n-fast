#ifndef _SNF_KEYMGR_H_
#define _SNF_KEYMGR_H_

#include "sslfcn.h"
#include <mutex>

namespace snf {
namespace net {
namespace ssl {

constexpr int KEY_SIZE  = 16;
constexpr int AES_SIZE  = 32;
constexpr int HMAC_SIZE = 16;

/*
 * Key record.
 */
struct keyrec
{
	uint8_t key_name[KEY_SIZE];     // Key name
	uint8_t aes_key[AES_SIZE];      // AES key
	uint8_t hmac_key[HMAC_SIZE];    // HMAX key
	time_t  expire;                 // Epoch time when the key expires
};

/*
 * Key manager interface.
 *
 * If Session ticket based SSL resumption is in used, the session-state
 * information is encrypted and sent back to the client in the form
 * of a ticket. The ticket is created by a TLS server and sent to a TLS
 * client. The TLS client presents the ticket to the TLS server to resume
 * a session.
 *
 * The key record, keyrec, is an implementation of the encryption key
 * record on the TLS server side. The key manager, keymgr, is a public
 * interface to create new key record or find an existing key.
 */
class keymgr
{
public:
	virtual ~keymgr() {}
	virtual const keyrec *get() = 0;
	virtual const keyrec *find(const uint8_t *, size_t) = 0;
};

/*
 * A basic key manager implementation.
 */
class basic_keymgr : public keymgr
{
private:
	int         m_life = 0;
	keyrec      *m_cur = nullptr;
	keyrec      *m_old = nullptr;
	std::mutex  m_lock;

public:
	basic_keymgr(int n = 3600) : m_life(n) { }

	basic_keymgr(const basic_keymgr &) = delete;
	basic_keymgr(basic_keymgr &&) = delete;

	~basic_keymgr()
	{
		if (m_old) { delete m_old; m_old = nullptr; }
		if (m_cur) { delete m_cur; m_cur = nullptr; }
	}

	const basic_keymgr &operator=(const basic_keymgr &) = delete;
	basic_keymgr &operator=(basic_keymgr &&) = delete;

	const keyrec *get();
	const keyrec *find(const uint8_t *, size_t);
};

} // namespace ssl
} // namespace net
} // namespace snf

#endif // _SNF_KEYMGR_H_
