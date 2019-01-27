#include "session.h"
#include "dbg.h"
#include "file.h"
#include "error.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace net {
namespace ssl {

/*
 * Constructs SSL session from in-memory session data.
 *
 * @param [in] data - session data.
 * @param [in] len  - session data length.
 *
 * @throws snf::net::ssl::exception if the SSL session could not
 *         be imported from the given data.
 */
session::session(const uint8_t *data, size_t len)
{
	m_session = ssl_library::instance().ssl_session_d2i()
			(nullptr, &data, static_cast<long>(len));
	if (m_session == nullptr)
		throw exception("failed to import SSL session");
}

/*
 * Constructs SSL session from the contents of the given file.
 *
 * @param [in] name - session file name.
 *
 * @throws std::system_error if the file could not be opened or read,
 *         snf::net::ssl::exception if the SSL session could not
 *         be imported from the file content.
 */
session::session(const std::string &name)
{
	int oserr = 0;
	snf::file f(name, 0022);
	snf::file::open_flags flags;

	flags.o_read = true;

	if (f.open(flags, 0600, &oserr) != E_ok) {
		std::ostringstream oss;
		oss << "failed to open file " << name;
		throw std::system_error(
			oserr,
			std::system_category(),
			oss.str());
	}

	int bread = 0;
	int len = static_cast<int>(f.size());
	uint8_t *data = DBG_NEW uint8_t[len];

	if (f.read(data, len, &bread, &oserr) != E_ok) {
		delete [] data;
		std::ostringstream oss;
		oss << "failed to read from file " << name;
		throw std::system_error(
			oserr,
			std::system_category(),
			oss.str());
	}

	const uint8_t *buf = data;
	m_session = ssl_library::instance().ssl_session_d2i()
			(nullptr, &buf, static_cast<long>(len));
	if (m_session == nullptr) {
		delete [] data;
		throw exception("failed to import SSL session");
	}

	delete [] data;

	f.close();
}

/*
 * Constructs the SSL session from the raw SSL_SESSION.
 * It bumps up the reference count of the SSL session.
 *
 * @param [in] s - raw SSL_SESSION.
 *
 * @throws snf::net::ssl::exception if the reference count could not be incremented.
 */
session::session(SSL_SESSION *s)
{
	if (ssl_library::instance().ssl_session_up_ref()(s) != 1)
		throw exception("failed to increment the SSL session reference count");
	m_session = s;
}

/*
 * Copy constructor. No copy is done, the class simply points to the same
 * raw SSL session and the reference count in bumped up.
 *
 * @param [in] s - SSL session.
 *
 * @throws snf::net::ssl::exception if the reference count could not be incremented.
 */
session::session(const session &s)
{
	if (ssl_library::instance().ssl_session_up_ref()(s.m_session) != 1)
		throw exception("failed to increment the SSL session reference count");
	m_session = s.m_session;
}

/*
 * Move constructor.
 */
session::session(session &&s)
{
	m_session = s.m_session;
	s.m_session = nullptr;
}

/*
 * Destructor. The reference count to the SSL session is decremented. If it is the
 * last reference, the SSL session is deleted.
 */
session::~session()
{
	if (m_session) {
		ssl_library::instance().ssl_session_free()(m_session);
		m_session = nullptr;
	}
}

/*
 * Copy operator. No copy is done, the class simply points to the same
 * raw SSL session and the reference count in bumped up.
 *
 * @throws snf::net::ssl::exception if the reference count could not be incremented.
 */
const session &
session::operator=(const session &s)
{
	if (this != &s) {
		if (ssl_library::instance().ssl_session_up_ref()(s.m_session) != 1)
			throw exception("failed to increment the SSL session reference count");
		if (m_session)
			ssl_library::instance().ssl_session_free()(m_session);
		m_session = s.m_session;
	}
	return *this;
}

/*
 * Move operator.
 */
session &
session::operator=(session &&s)
{
	if (this != &s) {
		if (m_session)
			ssl_library::instance().ssl_session_free()(m_session);
		m_session = s.m_session;
		s.m_session = nullptr;
	}
	return *this;
}

/*
 * Exports SSL session to in-memory data. The returned data must be freed using
 * 'delete []'.
 *
 * @param [out] len - session data length in case of success.
 *
 * @return pointer to the session data.
 *
 * @throws snf::net::ssl::exception if the SSL session could not be exported.
 */
uint8_t *
session::to_bytes(size_t *len)
{
	size_t exp_len = ssl_library::instance().ssl_session_i2d()(m_session, nullptr);
	uint8_t *exp_data = DBG_NEW uint8_t[exp_len];
	uint8_t *data = exp_data;

	if (len == 0)
		throw std::invalid_argument("session data length must be specified");

	*len = ssl_library::instance().ssl_session_i2d()(m_session, &exp_data);

	if (exp_len != *len) {
		delete [] data;
		std::ostringstream oss;
		oss << "SSL session export length mismatch; expected " << exp_len
			<< ", found " << *len;
		throw exception(oss.str());
	}

	if (data + *len != exp_data) {
		delete [] data;
		throw exception("SSL session export data mismatch");
	}

	return data;
}

/*
 * Exports SSL session to the given file.
 *
 * @param [in] name - SSL session file name.
 *
 * @throws std::system_error if the file could not be opened or written to,
 *         snf::net::ssl::exception if the SSL session could not be exported.
 */
void
session::to_file(const std::string &name)
{
	int oserr = 0;
	snf::file f(name, 0022);
	snf::file::open_flags flags;

	flags.o_write = true;
	flags.o_create = true;
	flags.o_truncate = true;

	if (f.open(flags, 0600, &oserr) != E_ok) {
		std::ostringstream oss;
		oss << "failed to open file " << name;
		throw std::system_error(
			oserr,
			std::system_category(),
			oss.str());
	}

	int bwritten = 0;
	size_t len = 0;
	uint8_t *data = to_bytes(&len);

	if (f.write(data, static_cast<int>(len), &bwritten, &oserr) != E_ok) {
		delete [] data;
		std::ostringstream oss;
		oss << "failed to write to file " << name;
		throw std::system_error(
			oserr,
			std::system_category(),
			oss.str());
	}

	delete [] data;

	f.close();
}

/*
 * Gets raw SSL session identifier. The returned data must be freed using 'delete []'.
 *
 * @param [out] len - length of the SSL session identifier.
 *
 * @return SSL session identifier.
 */
uint8_t *
session::get_id(size_t *len)
{
	uint8_t *data = nullptr;
	unsigned int plen = 0;

	if (len) *len = 0;

	const uint8_t *ptr = ssl_library::instance().ssl_session_get_id()(m_session, &plen);
	if (ptr && plen) {
		data = DBG_NEW uint8_t[plen];
		memcpy(data, ptr, plen);
		if (len) *len = plen;
	}

	return data;
}

/*
 * Gets SSL session identifier in hexadecimal form.
 */
std::string
session::get_id()
{
	size_t idlen = 0;
	uint8_t *id = get_id(&idlen);
	if (id && idlen) {
		std::string sid = std::move(snf::bin2hex(id, idlen));
		delete [] id;
		return sid;
	} else {
		return std::string {};
	}
}

/*
 * Gets the SSL session context. See snf::net::ssl::context.set_session_context() for
 * setting SSL session context.
 */
std::string
session::get_context()
{
	unsigned int plen = 0;
	const uint8_t *ptr = ssl_library::instance().ssl_session_get_id_ctx()(m_session, &plen);
	return std::string { reinterpret_cast<const char *>(ptr), static_cast<size_t>(plen) };
}

/*
 * Gets SSL session protocol version.
 */
int
session::get_protocol_version()
{
	return ssl_library::instance().ssl_session_get_protocol()(m_session);
}

/*
 * Gets SSL session start time.
 */
time_t
session::start_time()
{
	return static_cast<time_t>(ssl_library::instance().ssl_session_get_time()(m_session));
}

/*
 * Gets SSL session timeout in seconds.
 */
time_t
session::timeout()
{
	return static_cast<time_t>(ssl_library::instance().ssl_session_get_timeout()(m_session));
}

/*
 * Sets SSL session timeout in seconds.
 *
 * @param [in] to - timeout in seconds.
 *
 * @throws snf::net::ssl::exception if could not set the timeout.
 */
void
session::timeout(time_t to)
{
	long lto = static_cast<long>(to);
	if (ssl_library::instance().ssl_session_set_timeout()(m_session, lto) != 1)
		throw exception("failed to set the session timeout");
}

/*
 * Determines if the SSL session has tickets. See snf::net::ssl::context.session_ticket()
 * to enable session tickets.
 */
bool
session::has_ticket()
{
	return (ssl_library::instance().ssl_session_has_ticket()(m_session) == 1);
}

} // namespace ssl
} // namespace net
} // namespace snf
