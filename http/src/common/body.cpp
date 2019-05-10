#include "body.h"
#include <string>
#include <algorithm>
#include "file.h"
#include "status.h"
#include "error.h"
#include "dbg.h"

namespace snf {
namespace http {

class body_from_buffer : public body
{
private:
	size_t  m_buflen = 0;
	char    *m_buf = nullptr;
	char    *m_begin = nullptr;
	char    *m_end = nullptr;

public:
	body_from_buffer(void *buf, size_t buflen)
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
		if (m_begin < m_end) {
			buflen = m_buflen;
			m_begin += buflen;
			return m_buf;
		} else {
			buflen = 0;
			return nullptr;
		}
	}
};

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
		if (m_begin < m_end) {
			buflen = m_str.size();
			m_begin += buflen;
			return m_str.data();
		} else {
			buflen = 0;
			return nullptr;
		}
	}
};

class body_from_file : public body
{
private:
	std::string         m_filename;
	snf::file           *m_file = nullptr;
	size_t              m_read = 0;
	size_t              m_filesize = 0;
	char                m_buf[body::BUFSIZE];

public:
	body_from_file(const std::string filename)
		: m_filename(filename)
	{
		snf::file::open_flags oflags;
		oflags.o_read = true;
		int syserr = 0;

		m_file = DBG_NEW snf::file(m_filename, 0022);
		if (m_file->open(oflags, 0600, &syserr) != E_ok) {
			std::ostringstream oss;
			oss << "failed to open file " << m_filename << " for reading";
			throw std::system_error(
				syserr,
				std::system_category(),
				oss.str());
		} else {
			m_filesize = m_file->size();
		}
	}

	~body_from_file()
	{
		if (m_file)
			delete m_file;
	}

	size_t length() const { return m_filesize; }
	bool has_next() { return (m_read < m_filesize); }

	const void *next(size_t &buflen)
	{
		int bread = 0;
		int syserr = 0;

		if (m_file->read(m_buf, body::BUFSIZE, &bread, &syserr) != E_ok) {
			std::ostringstream oss;
			oss << "failed to read from file " << m_filename << " at index " << m_read;
			throw std::system_error(
				syserr,
				std::system_category(),
				oss.str());
		}

		if (bread) {
			m_read += bread;
			buflen = bread;
			return m_buf;
		} else {
			buflen = 0;
			return nullptr;
		}
	}
};

class body_from_functor : public body
{
private:
	body_functor_t      m_functor;
	size_t              m_read = 0;
	char                m_buf[body::BUFSIZE];

public:
	body_from_functor(body_functor_t &f)
		: m_functor(f)
	{
	}

	body_from_functor(body_functor_t &&f)
		: m_functor(std::move(f))
	{
	}

	bool chunked() const { return true; }

	bool has_next()
	{
		if (m_functor(m_buf, body::BUFSIZE, &m_read) != E_ok) {
			throw std::runtime_error("call to the functor failed");
		}
		return (m_read != 0);
	}

	const void *next(size_t &buflen)
	{
		if (m_read) {
			buflen = m_read;
			m_read = 0;
			return m_buf;
		} else {
			buflen = 0;
			return nullptr;
		}
	}
};

class body_from_socket : public body
{
private:
	snf::net::nio       *m_io = nullptr;
	size_t              m_size = 0;
	size_t              m_read = 0;
	char                m_buf[body::BUFSIZE];

public:
	body_from_socket(snf::net::nio *io, size_t len)
		: m_io(io)
		, m_size(len)
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

		if (m_io->read(m_buf, to_read, &bread, 1000, &syserr) != E_ok) {
			throw std::system_error(
				syserr,
				std::system_category(),
				"failed to read from socket");
		}

		if (bread) {
			m_read += bread;
			buflen = bread;
			return m_buf;
		} else {
			buflen = 0;
			return nullptr;
		}
	}
};

class body_from_socket_chunked : public body
{
private:
	snf::net::nio       *m_io = nullptr;
	size_t              m_chunk_size = 0;
	size_t              m_chunk_offset = 0;
	char                m_buf[body::BUFSIZE];

	int available_in_chunk()
	{
		return static_cast<int>(m_chunk_size - m_chunk_offset);
	}

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
	{
	}

	~body_from_socket_chunked() {}

	bool chunked() const { return true; }

	bool has_next()
	{
		int c1, c2;
		std::string hex;

		if (available_in_chunk() > 0)
			return true;

		while ((c1 = getc()) != '\r') {
			if (std::isxdigit(c1))
				hex.push_back(c1);
			else
				break;
		}

		if (hex.empty())
			throw http_exception("no chunk length", status_code::BAD_REQUEST);

		c2 = getc();
		if ((c1 != '\r') || (c2 != '\n'))
			throw http_exception(
				"chunk size line not terminated properly",
				status_code::BAD_REQUEST);

		m_chunk_size = std::stoll(hex, 0, 16);
		m_chunk_offset = 0;

		if (m_chunk_size == 0) {
			c1 = getc();
			c2 = getc();
			if ((c1 != '\r') || (c2 != '\n'))
				throw http_exception(
					"body not terminated properly",
					status_code::BAD_REQUEST);
		}

		return (available_in_chunk() > 0);
	}

	const void *next(size_t &buflen)
	{
		int to_read = std::min(body::BUFSIZE, available_in_chunk());
		int bread = 0;
		int syserr = 0;
		void *ptr = nullptr;

		if (m_io->read(m_buf, to_read, &bread, 1000, &syserr) != E_ok) {
			throw std::system_error(
				syserr,
				std::system_category(),
				"failed to read from socket");
		}

		if (bread) {
			m_chunk_offset += bread;
			buflen = bread;
			ptr = m_buf;
		} else {
			buflen = 0;
		}

		if (available_in_chunk() <= 0)
			if (('\r' != getc()) || ('\n' != getc()))
				throw http_exception(
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
