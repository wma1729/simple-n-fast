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

	void validate_length_chunkiness();
	virtual void validate();

public:
	message() {}

	message(const version &ver, const headers &hdrs, const std::shared_ptr<body> &b)
		: m_version(ver)
		, m_headers(hdrs)
		, m_body(b)
	{
	}

	message(const version &ver, headers &&hdrs, std::shared_ptr<body> &&b)
		: m_version(ver)
		, m_headers(std::move(hdrs))
		, m_body(b)
	{
	}

	message(const message &msg)
		: m_version(msg.m_version)
		, m_headers(msg.m_headers)
		, m_body(msg.m_body)
	{
	}

	message(message &&msg)
		: m_version(msg.m_version)
		, m_headers(std::move(msg.m_headers))
		, m_body(std::move(msg.m_body))
	{
	}

	virtual ~message() {}

	const message &operator=(const message &msg)
	{
		if (this != &msg) {
			m_version = msg.m_version;
			m_headers = msg.m_headers;
			m_body = msg.m_body;
		}
		return *this;
	}

	message &operator=(message &&msg)
	{
		if (this != &msg) {
			m_version = msg.m_version;
			m_headers = std::move(msg.m_headers);
			m_body = std::move(msg.m_body);
		}
		return *this;
	}

	const version & get_version() const { return m_version; }
	void set_version(const version &v) { m_version = v; }

	const headers & get_headers() const { return m_headers; }
	void set_headers(const headers &h) { m_headers = h; }
	void add_header(const std::string &line) { m_headers.add(line); }
	void add_header(const std::string &k, const std::string &v) { m_headers.add(k, v); }
	void update_header(const std::string &k, const std::string &v) { m_headers.update(k, v); }
	void remove_header(const std::string &k) { m_headers.remove(k); }

	bool has_body() const { return static_cast<bool>(m_body); }
	body *get_body() const { return m_body.get(); }
	void set_body(body *b) { m_body.reset(b); }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_MESSAGE_H_
