#ifndef _SNF_HTTP_HDR_FLD_VALUE_H_
#define _SNF_HTTP_HDR_FLD_VALUE_H_

#include <string>
#include <vector>
#include <utility>
#include "net.h"
#include "version.h"
#include "uri.h"
#include "timeutil.h"

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
	string_list_value &operator+=(const string_list_value &);
	std::string str() const;
};

using param_vec_t = std::vector<std::pair<std::string, std::string>>;

/*
 * Token name and option parameters.
 */
struct token
{
	std::string     m_name;         // name
	param_vec_t     m_parameters;   // optional parameters

	token() {}
	token(const std::string &name) : m_name(name) {}
	token(const std::string &name, const param_vec_t &parameters)
		: m_name(name), m_parameters(parameters) {}
	token(const token &t)
		: m_name(t.m_name), m_parameters(t.m_parameters) {}
	token(token &&t)
		: m_name(std::move(t.m_name)), m_parameters(std::move(t.m_parameters)) {}

	const token &operator=(const token &t)
	{
		if (this != &t) {
			m_name = t.m_name;
			m_parameters = t.m_parameters;
		}
		return *this;
	}

	token &operator=(token &&t)
	{
		if (this != &t) {
			m_name = std::move(t.m_name);
			m_parameters = std::move(t.m_parameters);
		}
		return *this;
	}
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
	token_list_value & operator+=(const token_list_value &);
	std::string str() const;
};

/*
 * Host name and port combination.
 */
struct host_port
{
	std::string     m_host;    // host name
	in_port_t       m_port;    // optional port number

	host_port() : m_port(0) {}
	host_port(const std::string &h, in_port_t p = 0)
		: m_host(h), m_port(0) {}
	host_port(const host_port &hp)
		: m_host(hp.m_host), m_port(hp.m_port) {}
	host_port(host_port &&hp)
		: m_host(std::move(hp.m_host)), m_port(hp.m_port) {}

	const host_port &operator=(const host_port &hp)
	{
		if (this != &hp) {
			m_host = hp.m_host;
			m_port = hp.m_port;
		}
		return *this;
	}

	host_port &operator=(host_port &&hp)
	{
		if (this != &hp) {
			m_host = std::move(hp.m_host);
			m_port = hp.m_port;
		}
		return *this;
	}
};

/*
 * Host header field value.
 */
class host_value : public header_field_value
{
private:
	host_port       m_hp;
	void            parse(const std::string &);

public:
	host_value(const std::string &);
	host_value(const std::string &, in_port_t);
	virtual ~host_value() {}
	const std::string &host() const { return m_hp.m_host; }
	in_port_t port() const { return m_hp.m_port; }
	const host_port &get() const { return m_hp; }
	std::string str() const;
};

/*
 * Media type (for Content-Type) header field.
 */
struct media_type
{
	std::string m_type;        // type
	std::string m_subtype;     // sub type
	param_vec_t m_parameters;  // optional parameters

	media_type() {}
	media_type(const std::string &type, const std::string &subtype)
		: m_type(type), m_subtype(subtype) {}
	media_type(const media_type &mt)
		: m_type(mt.m_type), m_subtype(mt.m_subtype)
		, m_parameters(mt.m_parameters) {}
	media_type(media_type &&mt)
		: m_type(std::move(mt.m_type)), m_subtype(std::move(mt.m_subtype))
		, m_parameters(std::move(mt.m_parameters)) {}

	const media_type &operator=(const media_type &mt)
	{
		if (this != &mt) {
			m_type = mt.m_type;
			m_subtype = mt.m_subtype;
			m_parameters = mt.m_parameters;
		}
		return *this;
	}

	media_type &operator=(media_type &&mt)
	{
		if (this != &mt) {
			m_type = std::move(mt.m_type);
			m_subtype = std::move(mt.m_subtype);
			m_parameters = std::move(mt.m_parameters);
		}
		return *this;
	}
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
	const std::string &type() const { return m_mt.m_type; }
	const std::string &subtype() const { return m_mt.m_subtype; }
	const param_vec_t &parameters() const { return m_mt.m_parameters; }
	std::string str() const;
};

/*
 * Via (version/uri).
 */
struct via
{
	version m_ver;
	uri	m_uri;

	via() {}
	via(const version &v, const uri &u) : m_ver(v), m_uri(u) {}
	via(version &&v, uri &&u) : m_ver(v), m_uri(std::move(u)) {}
	via(const via &v) : m_ver(v.m_ver), m_uri(v.m_uri) {}
	via(via &&v) : m_ver(v.m_ver), m_uri(std::move(v.m_uri)) {}

	const via &operator=(const via &v)
	{
		if (this != &v) {
			m_ver = v.m_ver;
			m_uri = v.m_uri;
		}
		return *this;
	}

	via &operator=(via &&v)
	{
		if (this != &v) {
			m_ver = v.m_ver;
			m_uri = std::move(v.m_uri);
		}
		return *this;
	}
};

/*
 * Via header field value.
 */
class via_list_value : public header_field_value
{
private:
	std::vector<via>    m_via_list;
	void                parse(const std::string &);

public:
	via_list_value(const std::string &);
	via_list_value(const via &);
	via_list_value(const std::vector<via> &);
	virtual ~via_list_value() {}
	const std::vector<via> &get() const { return m_via_list; }
	via_list_value & operator+=(const via_list_value &);
	std::string str() const;
};

/*
 * Date header field value.
 */
class date_value : public header_field_value
{
private:
	snf::datetime   *m_dt;

public:
	date_value(const std::string &);
	date_value(time_t);
	date_value(const snf::datetime &);
	virtual ~date_value() { delete m_dt; }
	const snf::datetime &get() const { return *m_dt; }
	std::string str() const;
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_HDR_FLD_VALUE_H_
