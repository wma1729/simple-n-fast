#ifndef _SNF_HTTP_BODY_H_
#define _SNF_HTTP_BODY_H_

#include <functional>
#include "nio.h"

namespace snf {
namespace http {

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

#endif // _SNF_HTTP_BODY_H_
