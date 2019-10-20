#ifndef _SNF_HTTP_HDR_FLD_VALUE_H_
#define _SNF_HTTP_HDR_FLD_VALUE_H_

#include <string>
#include <vector>
#include <utility>
#include "net.h"

namespace snf {
namespace http {

/*
 * Base header field value.
 */
class header_field_value
{
protected:
	std::string m_raw;

public:
	header_field_value() {}
	header_field_value(const std::string &str) : m_raw(str) {}
	header_field_value(std::string &&str) : m_raw(std::move(str)) {}
	virtual ~header_field_value() {}
	virtual const std::string &raw() const { return m_raw; }
	virtual std::string str() const = 0;
};

/*
 * Numeric header field value.
 */
class number_value : public header_field_value
{
private:
	size_t	m_number;

public:
	number_value(size_t num) : m_number(num) {}
	number_value(const std::string &);
	virtual ~number_value() {}
	size_t get() const { return m_number; }
	std::string str() const;
};

/*
 * String header field value
 */
class string_value : public header_field_value
{
public:
	string_value(const std::string &str) : header_field_value(str) {}
	string_value(std::string &&str): header_field_value(std::move(str)) {}
	virtual ~string_value() {}
	const std::string &get() const { return m_raw; }
	std::string str() const { return std::string(m_raw); }
};

/*
 * Comma separated list of string header field value.
 */
class string_list_value : public header_field_value
{
private:
	std::vector<std::string> m_strings;
	std::vector<std::string> parse(const std::string &);

public:
	string_list_value(const std::string &);
	string_list_value(const std::vector<std::string> &);
	virtual ~string_list_value() {}
	const std::vector<std::string> &get() const { return m_strings; }
	std::string str() const;
	string_list_value &operator+=(const string_list_value &);
};

using param_vec_t = std::vector<std::pair<std::string, std::string>>;

/*
 * Token name and option parameters.
 */
struct token
{
	std::string     name;           // name
	param_vec_t     parameters;     // optional parameters
};

/*
 * Comma separated list of taken header field value.
 */
class token_list_value : public header_field_value
{
private:
	std::vector<token> m_tokens;
	std::vector<token> parse(const std::string &);

public:
	token_list_value(const std::string &);
	token_list_value(const token &);
	token_list_value(const std::vector<token> &);
	virtual ~token_list_value() {}
	const std::vector<token> &get() const { return m_tokens; }
	std::string str() const;
	token_list_value & operator+=(const token_list_value &);
};

/*
 * Host header field value.
 */
class host_value : public header_field_value
{
private:
	std::string     m_host;
	in_port_t       m_port;
	void            parse(const std::string &);

public:
	host_value(const std::string &);
	host_value(const std::string &, in_port_t);
	virtual ~host_value() {}
	const std::string &host() const { return m_host; }
	in_port_t port() const { return m_port; }
	std::string str() const;
};

/*
 * Media type (for Content-Type) header field.
 */
struct media_type {
	std::string type;       // type
	std::string subtype;    // sub type
	param_vec_t parameters; // optional parameters
};

/*
 * Media type header field value.
 */
class media_type_value : public header_field_value
{
private:
	media_type  m_mt;
	void        parse(const std::string &);

public:
	media_type_value(const std::string &);
	media_type_value(const std::string &, const std::string &);
	media_type_value(const media_type &);
	media_type_value(media_type &&);
	virtual ~media_type_value() {}
	const media_type &get() const { return m_mt; }
	const std::string &type() const { return m_mt.type; }
	const std::string &subtype() const { return m_mt.subtype; }
	const param_vec_t &parameters() const { return m_mt.parameters; }
	std::string str() const;
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_HDR_FLD_VALUE_H_
