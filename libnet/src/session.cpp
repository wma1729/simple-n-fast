#include "session.h"
#include "dbg.h"
#include "file.h"
#include "error.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace net {
namespace ssl {

session::session(const uint8_t *data, size_t len)
{
	m_session = ssl_library::instance().ssl_session_d2i()
			(nullptr, &data, static_cast<long>(len));
	if (m_session == nullptr)
		throw ssl_exception("failed to import SSL session");
}

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
	uint8_t *data = new uint8_t[len];

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
		throw ssl_exception("failed to import SSL session");
	}

	delete [] data;

	f.close();
}

session::session(SSL_SESSION *s)
{
	if (ssl_library::instance().ssl_session_up_ref()(s) != 1)
		throw ssl_exception("failed to increment the SSL session reference count");
	m_session = s;
}

session::session(const session &s)
{
	if (ssl_library::instance().ssl_session_up_ref()(s.m_session) != 1)
		throw ssl_exception("failed to increment the SSL session reference count");
	m_session = s.m_session;
}

session::session(session &&s)
{
	m_session = s.m_session;
	s.m_session = nullptr;
}

session::~session()
{
	if (m_session) {
		ssl_library::instance().ssl_session_free()(m_session);
		m_session = nullptr;
	}
}

const session &
session::operator=(const session &s)
{
	if (this != &s) {
		if (ssl_library::instance().ssl_session_up_ref()(s.m_session) != 1)
			throw ssl_exception("failed to increment the SSL session reference count");
		if (m_session)
			ssl_library::instance().ssl_session_free()(m_session);
		m_session = s.m_session;
	}
	return *this;
}

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

uint8_t *
session::to_bytes(size_t *len)
{
	size_t exp_len = ssl_library::instance().ssl_session_i2d()(m_session, nullptr);
	uint8_t *exp_data = DBG_NEW uint8_t[exp_len];
	uint8_t *data = exp_data;

	*len = ssl_library::instance().ssl_session_i2d()(m_session, &exp_data);

	if (exp_len != *len) {
		delete [] data;
		std::ostringstream oss;
		oss << "SSL session export length mismatch; expected " << exp_len
			<< ", found " << *len;
		throw ssl_exception(oss.str());
	}

	if (data + *len != exp_data) {
		delete [] data;
		throw ssl_exception("SSL session export data mismatch");
	}

	return data;
}

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

uint8_t *
session::get_id(size_t *len)
{
	uint8_t *data = nullptr;
	unsigned int plen = 0;

	*len = 0;

	const uint8_t *ptr = ssl_library::instance().ssl_session_get_id()(m_session, &plen);
	if (ptr && plen) {
		data = DBG_NEW uint8_t[plen];
		memcpy(data, ptr, plen);
		*len = plen;
	}

	return data;
}

std::string
session::get_context()
{
	unsigned int plen = 0;
	const uint8_t *ptr = ssl_library::instance().ssl_session_get_id_ctx()(m_session, &plen);
	return std::string { reinterpret_cast<const char *>(ptr), static_cast<size_t>(plen) };
}

int
session::get_protocol_version()
{
	return ssl_library::instance().ssl_session_get_protocol()(m_session);
}

} // namespace ssl
} // namespace net
} // namespace snf
