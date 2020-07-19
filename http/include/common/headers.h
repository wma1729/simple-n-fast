#ifndef _SNF_HTTP_CMN_HEADERS_H_
#define _SNF_HTTP_CMN_HEADERS_H_

#include <string>
#include <utility>
#include <vector>
#include <ostream>
#include <memory>
#include "hval.h"

namespace snf {
namespace http {

static const std::string CONTENT_LENGTH("content-length");
static const std::string TRANSFER_ENCODING("transfer-encoding");
static const std::string TE("te");
static const std::string TRAILERS("trailers");
static const std::string HOST("host");
static const std::string VIA("via");
static const std::string CONNECTION("connection");
static const std::string CONTENT_TYPE("content-type");
static const std::string CONTENT_ENCODING("content-encoding");
static const std::string CONTENT_LANGUAGE("content-language");
static const std::string CONTENT_LOCATION("content-location");
static const std::string DATE("date");

static const std::string TRANSFER_ENCODING_CHUNKED("chunked");

using hdr_vec_t = std::vector<std::pair<std::string, std::shared_ptr<base_value>>>;

/*
 * HTTP headers. Maintained as a vector of key/value pair.
 */
class headers
{
private:
	hdr_vec_t m_headers;

	hdr_vec_t::iterator find(const std::string &);
	hdr_vec_t::const_iterator find(const std::string &) const;
	base_value *validate(const std::string &, const std::string &);

public:
	static std::string canonicalize_name(const std::string &);

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

	bool empty() const { return m_headers.empty(); }
	void add(const std::string &);
	void add(const std::string &, const std::string &);
	void update(const std::string &, const std::string &);
	void update(const std::string &, base_value *);
	void remove(const std::string &);
	bool is_set(const std::string &) const;
	const base_value *get(const std::string &) const;

	/*
	 * Content-Length: <size>
	 */
	size_t content_length() const;
	void content_length(size_t);
	void content_length(const std::string &);

	/*
	 * Transfer-Encoding: chunked
	 */
	const std::vector<token> &transfer_encoding() const;
	void transfer_encoding(const token &);
	void transfer_encoding(const std::vector<token> &);
	void transfer_encoding(const std::string &);
	bool is_message_chunked() const;

	/*
	 * TE: trailers
	 */
	const std::vector<token> &te() const;
	void te(const token &);
	void te(const std::vector<token> &);
	void te(const std::string &);
	bool has_trailers() const;

	/*
	 * Trailers: <list_of_header_fields>
	 */
	const std::vector<std::string> &trailers() const;
	void trailers(const std::string &);
	void trailers(const std::vector<std::string> &);

	/*
	 * Host: <host>[:<port>]
	 */
	const std::string &host(in_port_t *) const;
	void host(const std::string &, in_port_t port);

	/*
	 * Via: [<protocol-name>/]<protocol-version> <received-by> <comment>
	 */
	const std::vector<via> &intermediary() const;
	void intermediary(const via &);
	void intermediary(const std::vector<via> &);
	void intermediary(const std::string &);

	/*
	 * Connection: close
	 * Connection: keep-alive
	 * Connection: upgrade
	 */
	const std::vector<std::string> &connection() const;
	bool close_connection() const;
	void connection(const std::string &);
	void connection(const std::vector<std::string> &);

	/*
	 * Content-Type: text/plain;charset=utf-8
	 * Content-Type: application/json;charset=iso-8859-1
	 */
	const media_type &content_type() const;
	void content_type(const media_type &);
	void content_type(const std::string &, const std::string &);

	/*
	 * Content-Encoding: gzip
	 */
	const std::vector<std::string> &content_encoding() const;
	void content_encoding(const std::string &);

	/*
	 * Content-Lanaguage: en-US [, en-UK]
	 */
	const std::vector<std::string> &content_language() const;
	void content_language(const std::string &);
	void content_language(const std::vector<std::string> &);

	/*
	 * Content-Location: <uri>
	 */
	uri content_location() const;
	void content_location(const uri &);
	void content_location(const std::string &);

	/*
	 * Date: <imf-date>
	 */
	const snf::datetime &date() const;
	void date(time_t);
	void date(const snf::datetime &);
	void date(const std::string &);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_HEADERS_H_
