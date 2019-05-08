#ifndef _SNF_NET_BUF_H_
#define _SNF_NET_BUF_H_

#include <streambuf>
#include <stdexcept>
#include "nio.h"

namespace snf {
namespace net {

/*
 * Customized output streambuf for sockets.
 * It could be use as following:
 *
 * snf::net::outbuf buf(io, 2048); // buffer of size 2048
 * std::ostream ostrm(&buf);
 *
 * Thereafter ostrm can be used like an ordinaly std::ostream object.
 */
class outbuf : public std::streambuf
{
public:
	outbuf(nio *io, size_t bufsize = 512)
		: m_io(io)
		, m_bufsize(bufsize)
	{
		m_buf = new char[m_bufsize];
		setp(&m_buf[0], &m_buf[m_bufsize - 1]);
	}

	~outbuf()
	{
		sync();
		delete [] m_buf;
	}

protected:
	virtual int_type overflow(int_type) override
	{
		if (c != traits_type::eof()) {
			*pptr() = c;
			pbump(1);
		}

		return (flush() == traits_type::eof()) ? traits_type::eof() : c;
	}

	virtual std::streamsize xsputn(const char *, std::streamsize) override
	{
		std::streamsize free = epptr() - pptr();
		if (buflen < free) {
			memcpy(pptr(), buf, buflen);
			pbump(buflen);
			return buflen;
		} else {
			flush();

			int to_write = static_cast<int>(buflen);
			int bwritten = 0;
			int syserr = 0;

			if (m_io->write(buf, to_write, &bwritten, 1000, &syserr) != E_ok) {
				return 0;
			} else {
				return bwritten;
			}
		}
	}

	virtual int sync() override
	{
		int_type r = flush();
		return (r == traits_type::eof()) ? -1 : 0;
	}

private:
	nio     *m_io = nullptr;
	size_t  m_bufsize;
	char    *m_buf = nullptr;

	int_type flush()
	{
		int to_write = static_cast<int>(pptr() - pbase());
		int bwritten = 0;
		int syserr = 0;

		if (m_io->write(m_buf, to_write, &bwritten, 1000, &syserr) != E_ok) {
			return traits_type::eof();
		} else if (to_write != bwritten) {
			return traits_type::eof();
		} else {
			pbump(-to_write);
			return to_write;
		}
	}
};

/*
 * Customized input streambuf for sockets.
 * It could be use as following:
 *
 * snf::net::inbuf buf(io, 2048, 128); // buffer of size 2048, putback area of 128
 * std::istream istrm(&buf);
 *
 * Thereafter istrm can be used like an ordinaly std::istream object.
 */
class inbuf : public std::streambuf
{
public:
	inbuf(nio *io, size_t bufsize = 512, size_t putback_size = 64)
		: m_io(io)
		, m_bufsize(bufsize)
		, m_putback_size(putback_size)
	{
		if (putback_size > (bufsize / 2))
			throw std::invalid_argument("putback area is greater than the get area");

		m_buf = new char[bufsize];
		setg(
			&m_buf[m_putback_size],
			&m_buf[m_putback_size],
			&m_buf[m_putback_size]);
	}

	~inbuf()
	{
		delete [] m_buf;
	}

protected:
	virtual int_type underflow() override
	{
		if (gptr() >= egptr()) {
			size_t putback_count = std::min(static_cast<size_t>(gptr() - eback()), m_putback_size);
			if (putback_count > 0)
				std::memmove(
					&m_buf[m_putback_size - putback_count],
					gptr() - putback_count,
					putback_count);

			int to_read = static_cast<int>(m_bufsize - m_putback_size);
			int bread = 0;
			int syserr = 0;

			if (m_io->read(m_buf + m_putback_size, to_read, &bread, 1000, &syserr) != E_ok)
				return traits_type::eof();

			setg(
				&m_buf[m_putback_size - putback_count],
				&m_buf[m_putback_size],
				&m_buf[m_putback_size + bread]);
		}

		return traits_type::to_int_type(*gptr());
	}

private:
	nio     *m_io = nullptr;
	size_t  m_bufsize;
	size_t  m_putback_size;
	char    *m_buf = nullptr;
};

} // namespace net
} // namespace snf

#endif // _SNF_NET_BUF_H_
