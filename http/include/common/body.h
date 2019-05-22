#ifndef _SNF_HTTP_CMN_BODY_H_
#define _SNF_HTTP_CMN_BODY_H_

#include <functional>
#include "nio.h"

namespace snf {
namespace http {

/*
 * HTTP message, either request or response, body.
 * The factory class should be employed to get
 * HTTP body from different sources.
 *
 * To read the body data, the following style could
 * be employed:
 *
 * body *req_body = body_factory::instance().from_socket(io);
 * while (req_body->has_next) {
 *     size_t datalen = 0;
 *     const voud *data = req_body->next(datalen);
 *     // process data
 * }
 *
 */
class body
{
public:
	static const int BUFSIZE = 8192;

	virtual bool chunked() { return false; }
	virtual size_t length() { return 0; }
	virtual bool has_next() = 0;
	virtual const void *next(size_t &) = 0;
};

using body_functor_t = std::function<int(void *, size_t, size_t *)>;

/*
 * HTTP body factory.
 * A few common body creation methods are supplied:
 * - from_buffer:         body constructed from a buffer.
 * - from string:         body constructed from string.
 * - from_file:           body constructed from file content.
 * - from_functor:        body constructed from a callable.
 * - from socket:         body constructed from data read
 *                        from socket. The size of the data
 *                        to be read in known in advance,
 *                        most likely from Content-Length
 *                        header.
 * - from socket_chunked: body constructed from data read
 *                        from socket. The body is assumed
 *                        to be chunked probably determined
 *                        from Transfer-Encoding header.
 */
class body_factory
{
private:
	body_factory() {}

public:
	body_factory(const body_factory &) = delete;
	body_factory(body_factory &&) = delete;
	body_factory & operator=(const body_factory &) = delete;
	body_factory & operator=(body_factory &&) = delete;
	~body_factory();

	static body_factory & instance()
	{
		static body_factory factory;
		return factory;
	}

	body *from_buffer(void *, size_t);
	body *from_string(const std::string &);
	body *from_string(std::string &&);
	body *from_file(const std::string &);
	body *from_functor(body_functor_t &&);
	body *from_socket(snf::net::nio *, size_t);
	body *from_socket_chunked(snf::net::nio *);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_BODY_H_
