#ifndef _SNF_HTTP_CMN_HEADERS_H_
#define _SNF_HTTP_CMN_HEADERS_H_

#include <string>
#include <utility>
#include <vector>
#include <ostream>
#include <memory>
#include "hfval.h"

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

static const std::string TRANSFER_ENCODING_CHUNKED("chunked");

static const std::string CONNECTION_CLOSE("close");
static const std::string CONNECTION_KEEP_ALIVE("keep-alive");
static const std::string CONNECTION_UPGRADE("upgrade");

static const std::string CONTENT_TYPE_T_TEXT("text");
static const std::string CONTENT_TYPE_T_APPLICATION("application");
static const std::string CONTENT_TYPE_ST_PLAIN("plain");
static const std::string CONTENT_TYPE_ST_JSON("json");

static const std::string CONTENT_ENCODING_COMPRESS("compress");
static const std::string CONTENT_ENCODING_X_COMPRESS("x-compress");
static const std::string CONTENT_ENCODING_GZIP("gzip");
static const std::string CONTENT_ENCODING_X_GZIP("x-gzip");
static const std::string CONTENT_ENCODING_DEFLATE("deflate");

using hdr_vec_t = std::vector<std::pair<std::string, std::shared_ptr<header_field_value>>>;

/*
 * HTTP headers. Maintained as a vector of key/value pair.
 */
class headers
{
private:
	hdr_vec_t m_headers;

	hdr_vec_t::iterator find(const std::string &);
	hdr_vec_t::const_iterator find(const std::string &) const;
	header_field_value *validate(const std::string &, const std::string &);
	bool allow_comma_separated_values(const std::string &);

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
	void update(const std::string &, header_field_value *);
	void remove(const std::string &);
	bool is_set(const std::string &) const;
	const header_field_value *get(const std::string &) const;

	/*
	 * Content-Length: <size>
	 */
	size_t content_length() const;
	void content_length(size_t);

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
	void connection(const std::string &);

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

#if 0
	/*
	 * Content-Lanaguage: en-US [, en-UK]
	 */
	std::vector<std::string> content_language() const;
	void content_language(const std::string &);
	void content_language(const std::vector<std::string> &);
#endif
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_HEADERS_H_
