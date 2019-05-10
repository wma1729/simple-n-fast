#ifndef _SNF_HTTP_RESPONSE_H_
#define _SNF_HTTP_RESPONSE_H_

#include "status.h"
#include "message.h"
#include <string>

namespace snf {
namespace http {

class response_builder;

class response : public message
{
	friend class response_builder;

private:
	status_code m_status = status_code::OK;
	std::string m_reason;

	response() : message() {}

public:
	response(const response &resp)
		: message(resp.m_version, resp.m_headers, resp.m_body)
		, m_status(resp.m_status)
		, m_reason(resp.m_reason)
	{
	}

	response(response &&resp)
		: message(resp.m_version, std::move(resp.m_headers), std::move(resp.m_body))
		, m_status(resp.m_status)
		, m_reason(std::move(resp.m_reason))
	{
	}

	const response & operator=(const response &resp)
	{
		if (this != &resp) {
			m_version = resp.m_version;
			m_status = resp.m_status;
			m_body = resp.m_body;
			m_reason = resp.m_reason;
			m_headers = resp.m_headers;
		}
		return *this;
	}

	response & operator=(response &&resp)
	{
		if (this != &resp) {
			m_version = resp.m_version;
			m_status = resp.m_status;
			m_body = std::move(resp.m_body);
			m_reason = std::move(resp.m_reason);
			m_headers = std::move(resp.m_headers);
		}
		return *this;
	}

	const status_code & get_status() const { return m_status; }
	const std::string &get_reason() const { return m_reason; }
};

class response_builder
{
private:
	response m_response;

public:
	response_builder() {}

	response_builder & with_version(const version &v)
	{
		m_response.m_version = v;
		return *this;
	}

	response_builder & with_version(int major, int minor)
	{
		m_response.m_version.m_major = major;
		m_response.m_version.m_minor = minor;
		return *this;
	}

	response_builder & with_version(const std::string &str)
	{
		m_response.m_version = version(str);
		return *this;
	}

	response_builder & with_status(status_code status)
	{
		m_response.m_status = status;
		return *this;
	}

	response_builder & with_reason(const std::string &reason)
	{
		m_response.m_reason = reason;
		return *this;
	}

	response_builder & response_line(const std::string &);

	response_builder & with_headers(const headers &);

	response_builder & with_headers(headers &&);

	response_builder & with_body(std::unique_ptr<body> b)
	{
		m_response.m_body = std::move(b);
		return *this;
	}

	response build()
	{
		if (m_response.m_reason.empty())
			m_response.m_reason = reason_phrase(m_response.m_status);
		return std::move(m_response);
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_RESPONSE_H_
