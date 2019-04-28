#ifndef _SNF_HTTP_REQUEST_H_
#define _SNF_HTTP_REQUEST_H_

#include "method.h"
#include "uri.h"
#include "message.h"
#include <string>

namespace snf {
namespace http {

class request_builder;

class request : public message
{
	friend class request_builder;

private:
	method_type m_type = method_type::GET;
	uri         m_uri;

	request() : message() {}

public:
	request(const request &req)
		: message(req.m_version, req.m_headers)
		, m_type(req.m_type)
		, m_uri(req.m_uri)
	{
	}

	request(request &&req)
		: message(req.m_version, std::move(req.m_headers))
		, m_type(req.m_type)
		, m_uri(std::move(req.m_uri))
	{
	}

	const request & operator=(const request &req)
	{
		if (this != &req) {
			m_version = req.m_version;
			m_headers = req.m_headers;
			m_type = req.m_type;
			m_uri = req.m_uri;
		}
		return *this;
	}

	request & operator=(request &&req)
	{
		if (this != &req) {
			m_version = req.m_version;
			m_headers = std::move(req.m_headers);
			m_type = req.m_type;
			m_uri = std::move(req.m_uri);
		}
		return *this;
	}

	method_type get_method() const { return m_type; }
	const uri & get_uri() const { return m_uri; }
};

class request_builder
{
private:
	request m_request;

public:
	request_builder() {}

	request_builder & method(method_type type)
	{
		m_request.m_type = type;
		return *this;
	}

	request_builder & method(const std::string &str)
	{
		m_request.m_type = snf::http::method(str);
		return *this;
	}

	request_builder & with_uri(const uri &u)
	{
		m_request.m_uri = u;
		return *this;
	}

	request_builder & with_uri(uri &&u)
	{
		m_request.m_uri = std::move(u);
		return *this;
	}

	request_builder & with_version(const version &v)
	{
		m_request.m_version = v;
		return *this;
	}

	request_builder & with_version(int major, int minor)
	{
		m_request.m_version.m_major = major;
		m_request.m_version.m_minor = minor;
		return *this;
	}

	request_builder & with_version(const std::string &str)
	{
		m_request.m_version = version(str);
		return *this;
	}

	request_builder & request_line(const std::string &);

	request_builder & with_headers(const headers &);

	request_builder & with_headers(headers &&);

	request build()
	{
		return m_request;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_REQUEST_H_
