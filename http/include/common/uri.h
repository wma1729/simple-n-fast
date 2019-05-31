#ifndef _SNF_HTTP_CMN_URI_H_
#define _SNF_HTTP_CMN_URI_H_

#include "common.h"
#include "netplat.h"
#include <cctype>
#include <string>
#include <deque>
#include <ostream>
#include <algorithm>

namespace snf {
namespace http {

inline bool
uri_generic_delimiter(int c)
{
	bool delim = false;

	switch (c) {
		case ':':
		case '/':
		case '?':
		case '#':
		case '[':
		case ']':
		case '@':
			delim = true;
			break;

		default:
			break;
	}

	return delim;
}

inline bool
uri_subcomponent_delimiter(int c)
{
	bool delim = false;

	switch (c) {
		case '!':
		case '$':
		case '&':
		case '\'':
		case '(':
		case ')':
		case '*':
		case '+':
		case ',':
		case ';':
		case '=':
			delim = true;
			break;
		default:
			break;
	}

	return delim;
}

inline bool
uri_reserved_character(int c)
{
	return (uri_generic_delimiter(c)) ? true : uri_subcomponent_delimiter(c);
}

inline bool
uri_unreserved_character(int c)
{
	if (std::isalpha(c) || std::isdigit(c)) {
		return true;
	} else if ((c == '-') || (c == '.') || (c == '_') || (c == '~')) {
		return true;
	}

	return false;
}

inline bool
uri_percent_encoded(const std::string &str, size_t index)
{
	return ((str[index] == '%') &&
		std::isxdigit(str[index + 1]) &&
		std::isxdigit(str[index + 2]));
}

/*
 * Bad URI.
 */
class bad_uri : public std::runtime_error
{
public:
	bad_uri(const std::string &msg) : std::runtime_error(msg) { }
	bad_uri(const char *msg) : std::runtime_error(msg) { }
	virtual ~bad_uri() { }
};

class uri_component
{
protected:
	std::string m_component;

public:
	uri_component() {}
	uri_component(const std::string &comp) { set(comp); }
	uri_component(const uri_component &comp) { m_component = comp.m_component; }
	uri_component(uri_component &&comp) { m_component = std::move(comp.m_component); }
	virtual ~uri_component() {}

	virtual const uri_component & operator=(const uri_component &comp)
	{
		if (this != &comp)
			m_component = comp.m_component;
		return *this;
	}

	virtual uri_component & operator=(uri_component &&comp)
	{
		if (this != &comp)
			m_component = std::move(comp.m_component);
		return *this;
	}

	virtual bool is_present() const { return !m_component.empty(); }
	virtual bool is_valid(const std::string &) const = 0;
	virtual std::string encode(const std::string &) const;
	virtual std::string decode(const std::string &) const;
	virtual const std::string & get() const { return m_component; }
	virtual void set(const std::string &comp) { m_component = std::move(decode(comp)); }
};

inline std::ostream &
operator<<(std::ostream &os, const uri_component &comp)
{
	os << comp.encode(comp.get());
	return os;
}

class uri_scheme : public uri_component
{
public:
	using uri_component::uri_component;

	virtual ~uri_scheme() {}

	virtual bool is_valid(const std::string &) const;

	virtual std::string encode(const std::string &istr) const
	{
		std::string ostr;

		std::transform(
			istr.begin(), istr.end(), std::back_inserter(ostr),
			[](unsigned char c) {
				return std::tolower(c);
			}
		);

		return ostr;
	}

	virtual std::string decode(const std::string &istr) const
	{
		std::string ostr { istr };
		return ostr;
	}

	virtual void set(const std::string &);
};

class uri_userinfo : public uri_component
{
public:
	using uri_component::uri_component;

	virtual ~uri_userinfo() {}

	virtual bool is_valid(const std::string &) const;
	virtual std::string encode(const std::string &) const;
	virtual std::string decode(const std::string &) const;
};

class uri_host : public uri_component
{
private:
	bool is_regular_hostname(const std::string &) const;

public:
	using uri_component::uri_component;

	virtual ~uri_host() {}

	virtual bool is_valid(const std::string &) const;
	virtual std::string encode(const std::string &) const;
	virtual std::string decode(const std::string &) const;
	virtual void set(const std::string &);
};

class uri_port : public uri_component
{
public:
	using uri_component::uri_component;

	virtual ~uri_port() {}

	virtual bool is_valid(const std::string &) const;

	virtual std::string encode(const std::string &istr) const
	{
		return std::string { istr };
	}

	virtual std::string decode(const std::string &istr) const
	{
		return std::string { istr };
	}

	virtual void set(const std::string &);
};

class uri_path : public uri_component
{
private:
	std::string		m_path;
	std::deque<std::string> m_segments;
	bool                    m_slash_at_start = false;
	bool                    m_slash_at_end = false;

	std::string segments_to_path(const std::deque<std::string> &, bool, bool) const;

public:
	uri_path() {}
	uri_path(const std::string &str) : uri_component(str) {}

	uri_path(const uri_path &p)
		: uri_component(p.m_component)
	{
		m_path = p.m_path;
		m_segments = p.m_segments;
		m_slash_at_start = p.m_slash_at_start;
		m_slash_at_end = p.m_slash_at_end;
	}

	uri_path(uri_path &&p)
		: uri_component(std::move(p.m_component))
	{
		m_path = std::move(p.m_path);
		m_segments = std::move(p.m_segments);
		m_slash_at_start = p.m_slash_at_start;
		m_slash_at_end = p.m_slash_at_end;
	}

	virtual const uri_path & operator=(const uri_path &p)
	{
		if (this != &p) {
			m_component = p.m_component;
			m_path = p.m_path;
			m_segments = p.m_segments;
			m_slash_at_start = p.m_slash_at_start;
			m_slash_at_end = p.m_slash_at_end;
		}
		return *this;
	}

	virtual uri_path & operator=(uri_path &&p)
	{
		if (this != &p) {
			m_component = std::move(p.m_component);
			m_path = std::move(p.m_path);
			m_segments = std::move(p.m_segments);
			m_slash_at_start = p.m_slash_at_start;
			m_slash_at_end = p.m_slash_at_end;
		}
		return *this;
	}

	virtual ~uri_path() {}

	const std::string &get_path() const { return m_path; }
	bool is_absolute() const { return m_slash_at_start; }
	bool is_slash_at_end() const { return m_slash_at_end; }

	virtual bool is_valid(const std::string &) const;
	virtual std::string encode(const std::string &) const;
	virtual std::string decode(const std::string &) const;
	virtual void set(const std::string &);

	uri_path merge(const uri_path &) const;
};

class uri_query : public uri_component
{
public:
	using uri_component::uri_component;

	virtual ~uri_query() {}

	virtual bool is_valid(const std::string &) const;
};

class uri_fragment : public uri_component
{
public:
	using uri_component::uri_component;

	virtual ~uri_fragment() {}

	virtual bool is_valid(const std::string &) const;
};

class uri
{
public:
	uri() = default;
	uri(const std::string &str) { parse(str); }

	uri(const uri &u)
	{
		m_scheme = u.m_scheme;
		m_userinfo = u.m_userinfo;
		m_host = u.m_host;
		m_port = u.m_port;
		m_path = u.m_path;
		m_query = u.m_query;
		m_fragment = u.m_fragment;
	}

	uri(const uri &&u)
	{
		m_scheme = std::move(u.m_scheme);
		m_userinfo = std::move(u.m_userinfo);
		m_host = std::move(u.m_host);
		m_port = std::move(u.m_port);
		m_path = std::move(u.m_path);
		m_query = std::move(u.m_query);
		m_fragment = std::move(u.m_fragment);
	}

	const uri &operator=(const uri &u)
	{
		if (this != &u) {
			m_scheme = u.m_scheme;
			m_userinfo = u.m_userinfo;
			m_host = u.m_host;
			m_port = u.m_port;
			m_path = u.m_path;
			m_query = u.m_query;
			m_fragment = u.m_fragment;
		}
		return *this;
	}

	uri &operator=(uri &&u)
	{
		if (this != &u) {
			m_scheme = std::move(u.m_scheme);
			m_userinfo = std::move(u.m_userinfo);
			m_host = std::move(u.m_host);
			m_port = std::move(u.m_port);
			m_path = std::move(u.m_path);
			m_query = std::move(u.m_query);
			m_fragment = std::move(u.m_fragment);
		}
		return *this;
	}

	virtual ~uri() {}

	const uri_scheme &get_scheme() const { return m_scheme; }
	void set_scheme(const std::string &scheme) { m_scheme.set(scheme); }

	const uri_userinfo &get_userinfo() const { return m_userinfo; }
	void set_userinfo(const std::string &ui) { m_userinfo.set(ui); }

	const uri_host &get_host() const { return m_host; }
	void set_host(const std::string &host) { m_host.set(host); }

	const uri_port &get_port() const { return m_port; }
	void set_port(const std::string &port) { m_port.set(port); }

	const uri_path &get_path() const { return m_path; }
	void set_path(const std::string &path) { m_path.set(path); }

	const uri_query &get_query() const { return m_query; }
	void set_query(const std::string &query) { m_query.set(query); }

	const uri_fragment &get_fragment() const { return m_fragment; }
	void set_fragment(const std::string &frag) { m_fragment.set(frag); }

	uri merge(const uri &) const;

	friend std::ostream & operator<<(std::ostream &, const uri &);

private:
	uri_scheme      m_scheme;
	uri_userinfo    m_userinfo;
	uri_host        m_host;
	uri_port        m_port;
	uri_path        m_path;
	uri_query       m_query;
	uri_fragment    m_fragment;

	size_t parse_scheme(const std::string &, size_t);
	size_t parse_authority(const std::string &, size_t);
	size_t parse_path(const std::string &, size_t);
	size_t parse_query(const std::string &, size_t);
	size_t parse_fragment(const std::string &, size_t);

	void parse(const std::string &);
};

inline std::ostream &
operator<<(std::ostream &os, const uri &u)
{
	const uri_scheme &scheme = u.get_scheme();
	if (scheme.is_present())
		os << scheme << ":";

	const uri_host &host = u.get_host();
	if (host.is_present()) {
		os << "//";

		const uri_userinfo &ui = u.get_userinfo();
		if (ui.is_present())
			os << ui << "@";

		os << host;

		const uri_port &port = u.get_port();
		if (port.is_present())
			os << ":" << port;
	}

	const uri_path &path = u.get_path();
	if (path.is_present())
		os << path;

	const uri_query &query = u.get_query();
	if (query.is_present())
		os << "?" << query;

	const uri_fragment &fragment = u.get_fragment();
	if (fragment.is_present())
		os << "#" << fragment;

	return os;
}

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_URI_H_
