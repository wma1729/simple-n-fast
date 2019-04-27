#ifndef _SNF_HTTP_RQST_H_
#define _SNF_HTTP_RQST_H_

#include "method.h"
#include "uri.h"
#include "version.h"
#include "headers.h"
#include <string>

namespace snf {
namespace http {

class request_builder;

class request
{
	friend class request_builder;

private:
	method_type m_type = method_type::GET;
	uri         m_uri;
	version     m_version;
	headers     m_headers;

	request() {}

public:
	method_type get_method() const { return m_type; }
	const uri & get_uri() const { return m_uri; }
	const version & get_version() const { return m_version; }
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

	request_builder & with_header(const std::string &, const std::string &);

	request_builder & header_line(const std::string &);

	request build()
	{
		return m_request;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_RQST_H_
