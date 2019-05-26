#ifndef _SNF_HTTP_CMN_MESSAGE_H_
#define _SNF_HTTP_CMN_MESSAGE_H_

#include "version.h"
#include "headers.h"
#include "body.h"
#include <memory>

namespace snf {
namespace http {

/*
 * HTTP message. Common components of HTTP request and
 * response.
 */
class message
{
protected:
	version                 m_version;
	headers                 m_headers;
	std::shared_ptr<body>   m_body;

public:
	message() {}

	message(const version &ver, const headers &hdrs, const std::shared_ptr<body> &body)
		: m_version(ver)
		, m_headers(hdrs)
		, m_body(body)
	{
	}

	message(const version &ver, headers &&hdrs, std::shared_ptr<body> &&body)
		: m_version(ver)
		, m_headers(std::move(hdrs))
		, m_body(std::move(body))
	{
	}

	virtual ~message() {}

	const version & get_version() const { return m_version; }
	void set_version(const version &v) { m_version = v; }

	const headers & get_headers() const { return m_headers; }
	void set_headers(const headers &h) { m_headers = h; }
	void add_header(const std::string &line) { m_headers.add(line); }
	void add_header(const std::string &k, const std::string &v) { m_headers.add(k, v); }
	void update_header(const std::string &k, const std::string &v) { m_headers.update(k, v); }
	void remove_header(const std::string &k) { m_headers.remove(k); }

	body *get_body() const { return m_body.get(); }
	void set_body(body *b) { m_body.reset(b); }

	void validate();
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_MESSAGE_H_
