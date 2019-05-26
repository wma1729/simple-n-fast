#include "body.h"
#include <string>
#include <algorithm>
#include <stdexcept>
#include <ostream>
#include <sstream>
#include "file.h"
#include "status.h"
#include "error.h"
#include "dbg.h"

namespace snf {
namespace http {

/*
 * Get the HTTP body from the specified buffer.
 */
class body_from_buffer : public body
{
private:
	size_t  m_buflen = 0;
	char    *m_buf = nullptr;
	char    *m_begin = nullptr;
	char    *m_end = nullptr;

public:
	body_from_buffer(const void *buf, size_t buflen)
		: m_buflen(buflen)
	{
		m_buf = DBG_NEW char[m_buflen];
		memcpy(m_buf, buf, m_buflen);
		m_begin = static_cast<char *>(m_buf);
		m_end = m_begin + m_buflen;
	}

	~body_from_buffer()
	{
		delete [] m_buf;
	}

	size_t length() const { return m_buflen; }
	bool has_next() { return (m_begin < m_end); }

	const void *next(size_t &buflen)
	{
		const void *ptr = nullptr;

		if (m_begin < m_end) {
			buflen = m_buflen;
			m_begin += buflen;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		return ptr;
	}
};

/*
 * Get the HTTP body from the specified string.
 */
class body_from_string : public body
{
private:
	std::string     m_str;
	size_t          m_begin = 0;
	size_t          m_end = 0;

public:
	body_from_string(const std::string &str)
		: m_str(str)
		, m_begin(0)
	{
		m_end = m_str.size();
	}

	body_from_string(std::string &&str)
		: m_str(std::move(str))
		, m_begin(0)
	{
		m_end = m_str.size();
	}

	~body_from_string() {}

	size_t length() const { return m_str.size(); }
	bool has_next() { return (m_begin < m_end); }

	const void *next(size_t &buflen)
	{
		const void *ptr = nullptr;

		if (m_begin < m_end) {
			buflen = m_str.size();
			m_begin += buflen;
			ptr = m_str.data();
		} else {
			buflen = 0;
		}

		return ptr;
	}
};

/*
 * Get the HTTP body from the specified file.
 * Some of the operations can throw std::system_error
 * exception.
 */
class body_from_file : public body
{
private:
	snf::file           *m_file;
	std::string         m_filename;
	size_t              m_filesize;
	size_t              m_read;
	char                m_buf[body::BUFSIZE];

public:
	body_from_file(const std::string &filename)
		: m_filename(filename)
		, m_read(0)
	{
		snf::file::open_flags oflags;
		oflags.o_read = true;
		int syserr = 0;

		m_file = DBG_NEW snf::file(m_filename, 0022);
		if (m_file->open(oflags, 0600, &syserr) != E_ok) {
			std::ostringstream oss;
			oss << "failed to open file (" << m_filename << ") for reading";
			throw std::system_error(
				syserr,
				std::system_category(),
				oss.str());
		}

		m_filesize = m_file->size();
	}

	~body_from_file()
	{
		delete m_file;
	}

	size_t length() const { return m_filesize; }
	bool has_next() { return (m_read < m_filesize); }

	const void *next(size_t &buflen)
	{
		int bread = 0;
		int syserr = 0;
		const void *ptr = nullptr;

		if (m_file->read(m_buf, body::BUFSIZE, &bread, &syserr) != E_ok) {
			std::ostringstream oss;
			oss << "failed to read from file (" << m_filename << ") at index " << m_read;
			throw std::system_error(
				syserr,
				std::system_category(),
				oss.str());
		}

		if (bread) {
			m_read += bread;
			buflen = bread;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		return ptr;
	}
};

/*
 * Get the HTTP body from the specified functor.
 * Some of the operations can throw std::runtime_error
 * exception or any exception that the functor could
 * throw.
 */
class body_from_functor : public body
{
private:
	body_functor_t      m_functor;
	size_t              m_read;
	char                m_buf[body::BUFSIZE];

public:
	body_from_functor(body_functor_t &f)
		: m_functor(f)
		, m_read(0)
	{
	}

	body_from_functor(body_functor_t &&f)
		: m_functor(std::move(f))
		, m_read(0)
	{
	}

	bool chunked() const { return true; }

	bool has_next()
	{
		if (m_read != 0)
			return true;

		if (m_functor(m_buf, body::BUFSIZE, &m_read) != E_ok)
			throw std::runtime_error("call to the functor failed");

		return (m_read != 0);
	}

	const void *next(size_t &buflen)
	{
		const void *ptr = nullptr;

		if (m_read) {
			buflen = m_read;
			m_read = 0;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		return ptr;
	}
};

/*
 * Get the HTTP body from the specified file.
 * Some of the operations can throw std::system_error
 * exception.
 */
class body_from_socket : public body
{
private:
	snf::net::nio       *m_io;
	size_t              m_size;
	size_t              m_read;
	char                m_buf[body::BUFSIZE];

public:
	body_from_socket(snf::net::nio *io, size_t len)
		: m_io(io)
		, m_size(len)
		, m_read(0)
	{
	}

	~body_from_socket() {}

	size_t length() { return m_size; }
	bool has_next() { return m_read < m_size; }

	const void *next(size_t &buflen)
	{
		int to_read = static_cast<int>(m_size - m_read);
		to_read = std::min(body::BUFSIZE, to_read);
		int bread = 0;
		int syserr = 0;
		const void *ptr = nullptr;

		if (m_io->read(m_buf, to_read, &bread, 1000, &syserr) != E_ok)
			throw std::system_error(
				syserr,
				std::system_category(),
				"failed to read from socket");

		if (bread) {
			m_read += bread;
			buflen = bread;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		return ptr;
	}
};

/*
 * Get the HTTP body from the specified file.
 * Some of the operations can throw std::system_error
 * or snf::http::exception exception.
 */
class body_from_socket_chunked : public body
{
private:
	snf::net::nio       *m_io;
	size_t              m_chunk_size;
	size_t              m_chunk_offset;
	char                m_buf[body::BUFSIZE];

	int getc()
	{
		int syserr = 0;
		char c;

		if (m_io->get_char(c, 1000, &syserr) != E_ok)
			throw std::system_error(
				syserr,
				std::system_category(),
				"failed to read from socket");

		return c;
	}

public:
	body_from_socket_chunked(snf::net::nio *io)
		: m_io(io)
		, m_chunk_size(0)
		, m_chunk_offset(0)
	{
	}

	~body_from_socket_chunked() {}

	bool chunked() const { return true; }

	bool has_next()
	{
		int c;
		std::string hex;

		if (m_chunk_offset < m_chunk_size)
			return true;

		while ((c = getc()) != '\r') {
			if (std::isxdigit(c))
				hex.push_back(c);
			else
				break;
		}

		if (hex.empty())
			throw exception("no chunk size", status_code::BAD_REQUEST);

		if (('\r' != c) || ('\n' != getc()))
			throw exception(
				"chunk size line not terminated properly",
				status_code::BAD_REQUEST);

		m_chunk_size = std::stoll(hex, 0, 16);
		m_chunk_offset = 0;

		if (m_chunk_size == 0) {
			if (('\r' != getc()) || ('\n' != getc()))
				throw exception(
					"message body not terminated properly",
					status_code::BAD_REQUEST);
		}

		return (m_chunk_offset < m_chunk_size);
	}

	const void *next(size_t &buflen)
	{
		int to_read = static_cast<int>(m_chunk_size - m_chunk_offset);
		if (to_read > body::BUFSIZE)
			to_read = body::BUFSIZE;

		int bread = 0;
		int syserr = 0;
		const void *ptr = nullptr;

		if (m_io->read(m_buf, to_read, &bread, 1000, &syserr) != E_ok)
			throw std::system_error(
				syserr,
				std::system_category(),
				"failed to read from socket");

		if (bread) {
			m_chunk_offset += bread;
			buflen = bread;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		if (m_chunk_offset >= m_chunk_size)
			if (('\r' != getc()) || ('\n' != getc()))
				throw exception(
					"chunk data not terminated properly",
					status_code::BAD_REQUEST);

		return ptr;
	}
};

body *
body_factory::from_buffer(void *buf, size_t buflen)
{
	return DBG_NEW body_from_buffer(buf, buflen);
}

body *
body_factory::from_string(const std::string &str)
{
	return DBG_NEW body_from_string(str);
}

body *
body_factory::from_string(std::string &&str)
{
	return DBG_NEW body_from_string(std::move(str));
}

body *
body_factory::from_file(const std::string &filename)
{
	return DBG_NEW body_from_file(filename);
}

body *
body_factory::from_functor(body_functor_t &&f)
{
	return DBG_NEW body_from_functor(f);
}

body *
body_factory::from_socket(snf::net::nio *io, size_t length)
{
	return DBG_NEW body_from_socket(io, length);
}

body *
body_factory::from_socket_chunked(snf::net::nio *io)
{
	return DBG_NEW body_from_socket_chunked(io);
}

} // namespace http
} // namespace snf
