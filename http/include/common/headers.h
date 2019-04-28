#ifndef _SNF_HEADERS_H_
#define _SNF_HEADERS_H_

#include "common.h"
#include <string>
#include <map>
#include <functional>

namespace snf {
namespace http {

static const std::string CONTENT_LENGTH("Content-Length");

inline bool
name_cmp(const std::string &s1, const std::string &s2)
{
	return snf::streq(s1, s2, true);
}

class headers
{
private:
	using headers_map_t = std::map<std::string, std::string,
		std::function<bool(const std::string &, const std::string &)>>;
	headers_map_t m_headers;

public:
	headers() : m_headers(name_cmp) {}
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
	void update(const std::string &, const std::string &);
	bool is_set(const std::string &) const;
	std::string get(const std::string &) const;
	int64_t content_length() const;
	void content_length(int64_t);
};

} // namespace http
} // namespace snf

#endif // _SNF_HEADERS_H_
