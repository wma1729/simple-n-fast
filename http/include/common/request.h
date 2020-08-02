#ifndef _SNF_HTTP_CMN_REQUEST_H_
#define _SNF_HTTP_CMN_REQUEST_H_

#include <string>
#include <istream>
#include <map>
#include "method.h"
#include "uri.h"
#include "message.h"
#include "nio.h"

namespace snf {
namespace http {

class request_builder;

using path_param_t      = std::pair<std::string, std::string>;
using path_param_map_t  = std::map<std::string, std::string>;

/*
 * HTTP request.
 */
class request : public message
{
	friend class request_builder;

private:
	method_type         m_type = method_type::M_GET;
	uri                 m_uri;
	headers             m_trailers;
	path_param_map_t    m_parameters;

	request() : message() {}

protected:
	virtual void validate() override;

public:
	request(const request &req)
		: message(req.m_version, req.m_headers, req.m_body)
		, m_type(req.m_type)
		, m_uri(req.m_uri)
	{
	}

	request(request &&req)
		: message(req.m_version, std::move(req.m_headers), std::move(req.m_body))
		, m_type(req.m_type)
		, m_uri(std::move(req.m_uri))
	{
	}

	const request & operator=(const request &req)
	{
		if (this != &req) {
			message::operator=(req);
			m_type = req.m_type;
			m_uri = req.m_uri;
		}
		return *this;
	}

	request & operator=(request &&req)
	{
		if (this != &req) {
			message::operator=(std::move(req));
			m_type = req.m_type;
			m_uri = std::move(req.m_uri);
		}
		return *this;
	}

	method_type get_method() const { return m_type; }
	void set_method(method_type type) { m_type = type; }

	const uri & get_uri() const { return m_uri; }
	void set_uri(const uri &u) { m_uri = u; }

	const headers & get_trailers() const { return m_trailers; }
	void set_trailers(const headers & trailers) { m_trailers = trailers; }
	void set_trailers(headers && trailers) { m_trailers = std::move(trailers); }

	const path_param_map_t & get_path_parameters() const { return m_parameters; }
	void set_path_parameter(const std::string &key, const std::string &val)
		{ m_parameters.insert(std::make_pair(key, val)); }
	void set_path_parameter(const path_param_t &kv)
		{ m_parameters.insert(kv); }
};

inline std::ostream &
operator<<(std::ostream &os, const request &req)
{
	// request line
	os << method(req.get_method());
	os << " " << req.get_uri().get_path();
	if (req.get_uri().get_query().is_present())
		os << "?" << req.get_uri().get_query();
	if (req.get_uri().get_fragment().is_present())
		os << "#" << req.get_uri().get_fragment();
	os << " " << req.get_version() << "\r\n";

	// request headers
	os << req.get_headers() << "\r\n";

	return os;
}

/*
 * HTTP request builder.
 */
class request_builder
{
private:
	request m_request;

public:
	request_builder() {}
	request_builder(std::istream &, bool ignore_body = false);
	request_builder(snf::net::nio *, bool ignore_body = false);

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

	request_builder & with_uri(const std::string &str)
	{
		m_request.m_uri = std::move(uri(str));
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

	request_builder & with_headers(const headers &hdrs)
	{
		m_request.m_headers = hdrs;
		return *this;
	}

	request_builder & with_headers(headers &&hdrs)
	{
		m_request.m_headers = std::move(hdrs);
		return *this;
	}

	request_builder & with_body(body *b)
	{
		m_request.m_body.reset(b);
		return *this;
	}

	request build(bool validate = true)
	{
		if (validate)
			m_request.validate();
		return m_request;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_REQUEST_H_
