#ifndef _SNF_SESSION_H_
#define _SNF_SESSION_H_

#include "sslfcn.h"

namespace snf {
namespace ssl {

/*
 * Encapsulates OpenSSL session (SSL_SESSION).
 * - A type operator is provided to get the raw session.
 * - The session can be streamed to file or memory.
 * - Session attributes can be queried.
 */
class session
{
public:
	session(const uint8_t *, size_t);
	session(const std::string &);
	session(const session &);
	session(session &&);
	session(SSL_SESSION *);
	~session();

	const session &operator=(const session &);
	session &operator=(session &&);

	operator SSL_SESSION* () { return m_session; }

	uint8_t *to_bytes(size_t *);
	void to_file(const std::string &);

	uint8_t *get_id(size_t *);
	std::string get_id();
	std::string get_context();
	int get_protocol_version();
	time_t start_time();
	time_t timeout();
	void timeout(time_t);
	bool has_ticket();

private:
	SSL_SESSION *m_session = nullptr;
};

} // namespace ssl
} // namespace snf

#endif // _SNF_SESSION_H_
