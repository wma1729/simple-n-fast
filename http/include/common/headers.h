#ifndef _SNF_HEADERS_H_
#define _SNF_HEADERS_H_

#include <string>
#include <utility>
#include <vector>
#include <ostream>

namespace snf {
namespace http {

static const std::string CONTENT_LENGTH("Content-Length");
static const std::string TRANSFER_ENCODING("Transfer-Encoding");
static const std::string TE("TE");

using hdr_vec_t = std::vector<std::pair<std::string, std::string>>;

class headers
{
private:
	hdr_vec_t m_headers;

	hdr_vec_t::iterator find(const std::string &);
	hdr_vec_t::const_iterator find(const std::string &) const;
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

	friend std::ostream &operator<<(std::ostream &, const headers &);

	void add(const std::string &);
	void add(const std::string &, const std::string &);
	void update(const std::string &, const std::string &);
	bool is_set(const std::string &) const;
	std::string get(const std::string &) const;

	int64_t content_length() const;
	void content_length(int64_t);

	std::string transfer_encoding() const;
	void transfer_encoding(const std::string &);
	bool is_message_chunked() const;

	std::string te() const;
	void te(const std::string &);
	bool is_trailer_included() const;
};

} // namespace http
} // namespace snf

#endif // _SNF_HEADERS_H_
