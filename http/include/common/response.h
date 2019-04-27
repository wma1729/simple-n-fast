#ifndef _SNF_HTTP_RESPONSE_H_
#define _SNF_HTTP_RESPONSE_H_

#include "version.h"
#include "status.h"
#include "headers.h"
#include <string>

namespace snf {
namespace http {

class response_builder;

class response
{
	friend class response_builder;

private:
	version     m_version;
	status_code m_status = status_code::OK;
	std::string m_reason;
	headers     m_headers;

	response() {}

public:
	response(const response &resp)
		: m_version(resp.m_version)
		, m_status(resp.m_status)
		, m_reason(resp.m_reason)
		, m_headers(resp.m_headers)
	{
	}

	response(response &&resp)
		: m_version(resp.m_version)
		, m_status(resp.m_status)
		, m_reason(std::move(resp.m_reason))
		, m_headers(std::move(resp.m_headers))
	{
	}

	const response & operator=(const response &resp)
	{
		if (this != &resp) {
			m_version = resp.m_version;
			m_status = resp.m_status;
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
			m_reason = std::move(resp.m_reason);
			m_headers = std::move(resp.m_headers);
		}
		return *this;
	}

	const version & get_version() const { return m_version; }
	const status_code & get_status() const { return m_status; }
	const std::string &get_reason() const { return m_reason; }
	const headers & get_headers() const { return m_headers; }
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

	response_builder & with_header(const std::string &, const std::string &);

	response_builder & header_line(const std::string &);

	response build()
	{
		if (m_response.m_reason.empty())
			m_response.m_reason = reason_phrase(m_response.m_status);
		return m_response;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_RESPONSE_H_
