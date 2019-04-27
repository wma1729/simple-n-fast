#ifndef _SNF_HEADERS_H_
#define _SNF_HEADERS_H_

#include <string>
#include <map>

namespace snf {
namespace http {

class headers
{
private:
	std::map<std::string, std::string> m_headers;

public:
	headers() {}
	headers(const headers &hdrs) { m_headers = hdrs.m_headers; }
	headers(headers &&hdrs) { m_headers = std::move(hdrs.m_headers); }

	const headers & operator=(const headers &hdrs)
	{
		if (this != &hdrs)
			m_headers = hdrs.m_headers;
		return *this;
	}

	headers & operator=(headers &&hdrs)
	{
		if (this != &hdrs)
			m_headers = std::move(hdrs.m_headers);
		return *this;
	}

	void add(const std::string &);
	void add(const std::string &, const std::string &);
	bool is_set(const std::string &);
	std::string get(const std::string &);
};

} // namespace http
} // namespace snf

#endif // _SNF_HEADERS_H_
