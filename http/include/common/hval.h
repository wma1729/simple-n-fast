#ifndef _SNF_HTTP_HDR_VALUE_H_
#define _SNF_HTTP_HDR_VALUE_H_

#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <functional>
#include "net.h"
#include "timeutil.h"
#include "scanner.h"
#include "version.h"
#include "uri.h"

namespace snf {
namespace http {

void parse(size_t &, const std::string &);
void parse(std::string &, const std::string &);
void parse(std::vector<std::string> &, const std::string &);
void parse(uri &, const std::string &);

/*
 * Token name and option parameters.
 */
struct token
{
	std::string     name;           // token name; lower case
	param_vec_t     parameters;     // optional parameters

	token() = default;
	token(const std::string &n) : name(n) {}
	token(const token &) = default;
	token(token &&) = default;
	token & operator = (const token &) = default;
	token & operator = (token &&) = default;

	bool operator == (const token &t) const { return name == t.name; }
};

inline std::ostream &
operator << (std::ostream &os, const token &t)
{
	os << t.name;
	for (auto p : t.parameters) {
		os << ";" << p.first;
		if (!p.second.empty())
			os << "=" << p.second;
	}
	return os;
}

void parse(std::vector<token> &, const std::string &);

/*
 * Host name and port combination.
 */
struct host_port
{
	std::string     host;           // host name
	in_port_t       port;           // optional port number

	host_port() : port(0) {}
};

inline std::ostream &
operator << (std::ostream &os, const host_port &hp)
{
	os << hp.host;
	if (hp.port)
		os << ":" << hp.port;
	return os;
}

void parse(host_port &, const std::string &);

/*
 * Media type (for Content-Type) header field.
 */
struct media_type
{
	std::string type;               // type
	std::string subtype;            // sub type
	param_vec_t parameters;         // optional parameters

	media_type() = default;
	media_type(const std::string &t, const std::string &st) : type(t), subtype(st) {}
	media_type(const media_type &) = default;
	media_type(media_type &&) = default;
	media_type & operator = (const media_type &) = default;
	media_type & operator = (media_type &&) = default;
};

inline std::ostream &
operator << (std::ostream &os, const media_type &mt)
{
	os << mt.type << "/" << mt.subtype;
	for (auto p : mt.parameters) {
		os << ";" << p.first;
		if (!p.second.empty())
			os << "=" << p.second;
	}
	return os;
}

void parse(media_type &mt, const std::string &);

/*
 * Via
 */
struct via
{
	version         ver;            // [protocol/] version
	uri	        url;            // message came via this URI
	std::string     comments;       // comments if any
};

inline std::ostream &
operator << (std::ostream &os, const via &v)
{
	os << v.ver << " ";

	if (v.url.get_host().is_present()) {
		os << v.url.get_host().get();
		if (v.url.get_port().is_present())
			os << ":" << v.url.get_port().numeric_port();
	}

	if (!v.comments.empty())
		os << " " << v.comments;

	return os;
}

void parse(std::vector<via> &, const std::string &);

inline std::ostream &
operator << (std::ostream &os, const snf::datetime &dt)
{
	os << dt.str(snf::time_format::imf);
	return os;
}

void parse(snf::datetime &, const std::string &);

/*
 * Base header value. Holds the header value in raw format (as received).
 * This is also needed so that all derived value classes can be store in
 * a container class.
 */
class base_value
{
protected:
	std::string m_raw;

public:
	base_value() {}
	base_value(const std::string &str) : m_raw(str) {}
	base_value(const base_value &) = default;
	base_value(base_value &&) = default;
	base_value & operator = (const base_value &) = default;
	base_value & operator = (base_value &&) = default;
	virtual ~base_value() {}
	const std::string &raw() const { return m_raw; }
	virtual bool is_seq() const = 0;
	virtual std::string str() const = 0;
};

template<typename V>
using Validator = void(*)(const V &);

template<typename T1, typename T2 = void>
using EnableIfNotString = typename std::enable_if<
				!std::is_convertible<T1, std::string>::value, T2
				>::type;

/*
 * Base singular header value.
 */
template<typename T, Validator<T> V>
class single_value : public base_value
{
protected:
	T m_value;

public:
	single_value() : base_value() {}

	single_value(const std::string &s) : base_value(s)
	{
		parse(m_value, s);
		(*V)(m_value);
	}

	template<typename NS>
	single_value(const NS &v, EnableIfNotString<NS> * = nullptr) : base_value() { set(v); }

	template<typename NS>
	single_value(NS &&v, EnableIfNotString<NS> * = nullptr) : base_value() { set(std::move(v)); }

	single_value(const single_value &) = default;
	single_value(single_value &&) = default;
	single_value & operator = (const single_value &) = default;
	single_value & operator = (single_value &&) = default;

	virtual ~single_value() {}

	bool is_seq() const override { return false; }

	const T & get() const { return m_value; }

	void set(const T &v)
	{
		(*V)(v);
		m_value = v;
	}

	void set(const T &&v)
	{
		(*V)(v);
		m_value = std::move(v);
	}

	std::string str() const override
	{
		std::ostringstream oss;
		oss << *this;
		return oss.str();
	}

	friend std::ostream & operator << (std::ostream &os, const single_value & v)
	{
		os << v.get();
		return os;
	}
};

/*
 * Base plural header value.
 */
template<typename T, Validator<T> V>
class sequence_value : public base_value
{
protected:
	std::vector<T> m_value;

public:
	sequence_value() : base_value() {}

	sequence_value(const std::string &s) : base_value(s)
	{
		parse(m_value, s);
		for (auto & v : m_value)
			(*V)(v);
	}

	template<typename NS>
	sequence_value(const NS &v, EnableIfNotString<NS> * = nullptr) : base_value() { append(v); }

	template<typename NS>
	sequence_value(NS &&v, EnableIfNotString<NS> * = nullptr) : base_value() { append(std::move(v)); }

	sequence_value(const std::vector<T> &vseq) : base_value() { append(vseq); }

	sequence_value(const sequence_value &) = default;
	sequence_value(sequence_value &&) = default;
	sequence_value & operator = (const sequence_value &) = default;
	sequence_value & operator = (sequence_value &&) = default;

	virtual ~sequence_value() {}

	bool is_seq() const override { return true; }

	const std::vector<T> & get() const { return m_value; }

	void append(const T &v)
	{
		(*V)(v);
		m_value.push_back(v);
	}

	void append(const T &&v)
	{
		(*V)(v);
		m_value.push_back(std::move(v));
	}

	void append(const std::vector<T> &vseq)
	{
		for (auto v : vseq) {
			(*V)(v);
			m_value.push_back(v);
		}
	}

	auto begin()        -> decltype(m_value.begin())    { return m_value.begin(); }
	auto end()          -> decltype(m_value.end())      { return m_value.end(); }
	auto cbegin() const -> decltype(m_value.cbegin())   { return m_value.cbegin(); }
	auto cend() const   -> decltype(m_value.cend())     { return m_value.cend(); }

	auto operator += (const sequence_value & v) -> decltype(*this)
	{
		if (!m_raw.empty())
			m_raw += ", ";
		m_raw += v.raw();
		m_value.insert(end(), v.cbegin(), v.cend());
		return *this;
	}

	std::string str() const override
	{
		std::ostringstream oss;
		oss << *this;
		return oss.str();
	}

	friend std::ostream & operator << (std::ostream &os, const sequence_value & v)
	{
		for (auto it = v.cbegin(); it != v.cend(); ++it) {
			if (it != v.cbegin())
				os << ", ";
			os << *it;
		}
		return os;
	}

};

static const std::string CONNECTION_CLOSE("close");
static const std::string CONNECTION_KEEP_ALIVE("keep-alive");
static const std::string CONNECTION_UPGRADE("upgrade");

void valid_connection(const std::string &);

static const std::string CONTENT_TYPE_T_TEXT("text");
static const std::string CONTENT_TYPE_T_APPLICATION("application");
static const std::string CONTENT_TYPE_ST_PLAIN("plain");
static const std::string CONTENT_TYPE_ST_JSON("json");

void valid_media_type(const media_type &);

static const std::string CONTENT_ENCODING_COMPRESS("compress");
static const std::string CONTENT_ENCODING_X_COMPRESS("x-compress");
static const std::string CONTENT_ENCODING_GZIP("gzip");
static const std::string CONTENT_ENCODING_X_GZIP("x-gzip");
static const std::string CONTENT_ENCODING_DEFLATE("deflate");

void valid_encoding(const std::string &);

template<typename T>
void
no_validation(const T &)
{
}

using num_single_val_t  = single_value <size_t,         no_validation>;
using mt_single_val_t   = single_value <media_type,     valid_media_type>;
using hp_single_val_t   = single_value <host_port,      no_validation>;
using uri_single_val_t  = single_value <uri,            no_validation>;
using dt_single_val_t   = single_value <snf::datetime,  no_validation>;
using ce_seq_val_t      = sequence_value<std::string,   valid_encoding>;
using str_seq_val_t     = sequence_value<std::string,   no_validation>;
using tok_seq_val_t     = sequence_value<token,         no_validation>;
using cnxn_seq_val_t    = sequence_value<std::string,   valid_connection>;
using vai_seq_val_t     = sequence_value<via,           no_validation>;

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_HDR_VALUE_H_
