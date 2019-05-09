#ifndef _SNF_HTTP_MESSAGE_H_
#define _SNF_HTTP_MESSAGE_H_

#include "version.h"
#include "headers.h"
#include <functional>

namespace snf {
namespace http {

class message
{
protected:
	version     m_version;
	headers     m_headers;

public:
	message() {}

	message(const version &ver, const headers &hdrs)
		: m_version(ver)
		, m_headers(hdrs)
	{
	}

	message(const version &ver, headers &&hdrs)
		: m_version(ver)
		, m_headers(std::move(hdrs))
	{
	}

	virtual ~message() {}

	const version & get_version() const { return m_version; }
	const headers & get_headers() const { return m_headers; }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_MESSAGE_H_
