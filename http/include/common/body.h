#ifndef _SNF_HTTP_CMN_BODY_H_
#define _SNF_HTTP_CMN_BODY_H_

#include <functional>
#include <sstream>
#include <ostream>
#include "nio.h"
#include "scanner.h"

/*
 * From RFC 7230:
 *
 * The presence of a message body in a request is signaled by a
 * Content-Length or Transfer-Encoding header field.
 *
 * Responses to the HEAD request method never include a message body
 * because the associated response header fields (e.g., Transfer-Encoding,
 * Content-Length, etc.), if present, indicate only what their values
 * would have been if the request method had been GET.
 *
 * 2xx (Successful) responses to a CONNECT request method switch to tunnel
 * mode instead of having a message body.
 *
 * All 1xx (Informational), 204 (No Content), and 304 (Not Modified)
 * responses do not include a message body. All other responses do
 * include a message body, although the body might be of zero length.
 */

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
 * while (req_body->has_next()) {
 *     size_t datalen = 0;
 *     const void *data = req_body->next(datalen);
 *     // process data
 * }
 *
 */
class body
{
public:
	static const int CHUNKSIZE = 65536;

	virtual ~body() {}

	virtual size_t length() const { return 0; }
	virtual bool chunked() const { return false; }
	virtual size_t chunk_size() const { return 0; }
	virtual param_vec_t chunk_extensions() { return param_vec_t(); }
	virtual bool has_next() = 0;
	virtual const void *next(size_t &) = 0;
};

using body_functor_t = std::function<int(void *, size_t, size_t *, param_vec_t *)>;

/*
 * HTTP body factory.
 *
 * A few common body creation methods are supplied.
 *
 * For unchunked body:
 * - from_buffer:         body constructed from a buffer.
 * - from string:         body constructed from string.
 * - from_file:           body constructed from file content.
 *
 * For chunked body:
 * - from_functor:        body constructed from a callable.
 *
 * For chunked/unchunked body:
 * - from_istream:        body constructed from input stream.
 * - from_socket:         body constructed from socket.
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
	body *from_istream(std::istream &, size_t);
	body *from_istream(std::istream &);
	body *from_socket(snf::net::nio *, size_t);
	body *from_socket(snf::net::nio *);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_BODY_H_
