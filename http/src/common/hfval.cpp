#include "hfval.h"
#include "uri.h"
#include "status.h"
#include "charset.h"
#include "parseutil.h"

namespace snf {
namespace http {

number_value::number_value(const std::string &istr)
	: header_field_value(istr)
{
	for (size_t i = 0; i < istr.length(); ++i) {
		if (!isdigit(istr[i])) {
			std::ostringstream oss;
			oss << "incorrect number (" << istr << ") specified";
			throw bad_message(oss.str());
		}
	}

	m_number = std::stoull(istr);
}

std::string
number_value::str() const
{
	return std::to_string(m_number);
}

std::vector<std::string>
string_list_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();
	std::vector<std::string> strvec;

	while (i < len) {
		std::string s;

		s = std::move(parse_token(istr, i, len));

		if (s.empty()) {
			std::ostringstream oss;
			oss << "no string found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		strvec.push_back(std::move(s));

		skip_spaces(istr, i, len);

		if (i >= len)
			break;

		if (istr[i] == ',') {
			i++;
		} else {
			std::ostringstream oss;
			oss << "strings not delimited properly in (" << istr << ")";
			throw bad_message(oss.str());
		}
	}

	return strvec;
}

string_list_value::string_list_value(const std::string &istr)
	: header_field_value(istr)
{
	m_strings = std::move(parse(istr));
}

string_list_value::string_list_value(const std::vector<std::string> &strings)
{
	for (auto s : strings)
		m_strings.push_back(s);
}

string_list_value &
string_list_value::operator+=(const string_list_value &slval)
{
	if (!slval.raw().empty()) {
		m_raw += ",";
		m_raw += slval.raw();
	}

	for (auto s : slval.get())
		m_strings.push_back(s);

	return *this;
}

std::string
string_list_value::str() const
{
	std::ostringstream oss;
	bool first = true;

	for (auto s : m_strings) {
		if (!first)
			oss << ", ";
		oss << s;
		first = false;
	}

	return oss.str();
}

std::vector<token>
token_list_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();
	std::vector<token> tokens;

	while (i < len) {
		token t;

		t.m_name = std::move(parse_token(istr, i, len));

		if (t.m_name.empty()) {
			std::ostringstream oss;
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		skip_spaces(istr, i, len);

		if ((i < len) && (istr[i] == ';'))
			t.m_parameters = std::move(parse_parameter(istr, i, len));

		tokens.push_back(std::move(t));

		skip_spaces(istr, i, len);

		if (i >= len)
			break;

		if (istr[i] == ',') {
			i++;
		} else {
			std::ostringstream oss;
			oss << "tokens not delimited properly in (" << istr << ")";
			throw bad_message(oss.str());
		}
	}

	return tokens;
}

token_list_value::token_list_value(const std::string &istr)
	: header_field_value(istr)
{
	m_tokens = std::move(parse(istr));
}

token_list_value::token_list_value(const token &token)
{
	m_tokens.push_back(token);
}

token_list_value::token_list_value(const std::vector<token> &tokens)
{
	for (auto t : tokens)
		m_tokens.push_back(t);
}

token_list_value &
token_list_value::operator+=(const token_list_value &tlval)
{
	if (!tlval.raw().empty()) {
		m_raw += ",";
		m_raw += tlval.raw();
	}

	for (auto t : tlval.get())
		m_tokens.push_back(t);

	return *this;
}

std::string
token_list_value::str() const
{
	std::ostringstream oss;
	bool first = true;

	for (auto t : m_tokens) {
		if (!first)
			oss << ", ";

		oss << t.m_name;
		for (auto p : t.m_parameters) {
			oss << ";" << p.first;
			if (!p.second.empty())
				oss << "=" << p.second;
		}

		first = false;
	}

	return oss.str();
}

void
host_value::parse(const std::string &hoststr)
{
	try {
		std::ostringstream oss;
		oss << "http://" << hoststr;
		std::string uristr = oss.str();

		snf::http::uri the_uri(uristr);

		if (the_uri.get_host().is_present()) {
			m_hp.m_host = the_uri.get_host().get();
		} else {
			oss.str("");
			oss << "invalid host string specified: " << hoststr;
			throw bad_message(oss.str());
		}

		if (the_uri.get_port().is_present()) {
			m_hp.m_port = the_uri.get_port().numeric_port();
		}
	} catch (snf::http::bad_uri &ex) {
		throw bad_message(ex.what());
	}
}

host_value::host_value(const std::string &hoststr)
	: header_field_value(hoststr)
{
	m_hp.m_port = 0;
	parse(hoststr);
}

host_value::host_value(const std::string &hoststr, in_port_t port)
	: header_field_value(hoststr)
{
	parse(hoststr);
	if (port) {
		m_raw += ":";
		m_raw += std::to_string(port);
		m_hp.m_port = port;
	}
}

std::string
host_value::str() const
{
	std::ostringstream oss;
	oss << m_hp.m_host;
	if (m_hp.m_port)
		oss << ":" << m_hp.m_port;
	return oss.str();
}

void
media_type_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();

	m_mt.m_type = std::move(parse_token(istr, i, len));

	if (m_mt.m_type.empty())
		throw bad_message("no type found");
	else if (istr[i] != '/')
		throw bad_message("type is not followed by /");
	else
		i++;

	m_mt.m_subtype = std::move(parse_token(istr, i, len));

	if (m_mt.m_subtype.empty())
		throw bad_message("no subtype found");
	else if ((i < len) && ((istr[i] == ';') || is_whitespace(istr[i])))
		m_mt.m_parameters = std::move(parse_parameter(istr, i, len));
	else
		throw bad_message("invalid character after subtype");
}

media_type_value::media_type_value(const std::string &istr)
	: header_field_value(istr)
{
	parse(istr);
}

media_type_value::media_type_value(const std::string &type, const std::string &subtype)
	: m_mt(type, subtype)
{
}

media_type_value::media_type_value(const media_type &mt)
	: m_mt(mt)
{
}

media_type_value::media_type_value(media_type &&mt)
	: m_mt(std::move(mt))
{
}

std::string
media_type_value::str() const
{
	std::ostringstream oss;

	oss << m_mt.m_type << "/" << m_mt.m_subtype;
	for (auto elem : m_mt.m_parameters) {
		oss << ";" << elem.first;
		if (!elem.second.empty())
			oss << "=" << elem.second;
	}

	return oss.str();
}

void
via_list_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();
	std::ostringstream oss;

	while (i < len) {
		via elem;

		skip_spaces(istr, i, len);

		if (i >= len)
			break;

		if (istr[i] == ',')
			++i;

		skip_spaces(istr, i, len);

		if (i >= len)
			break;

		size_t n = istr.find_first_of(' ', i);
		if (std::string::npos == n) {
			oss << "invalid via field value (" << istr << ")";
			throw bad_message(oss.str());
		} else {
			version v(istr.substr(i, (n - i)), true);
			elem.m_ver = v;
			i = n;
		}

		skip_spaces(istr, i, len);

		std::string hoststr;
		while (i < len) {
			if ((istr[i] == ',') || is_whitespace(istr[i]))
				break;
			else
				hoststr.push_back(istr[i++]);
		}

		if (hoststr.empty()) {
			oss << "missing received-by host (" << istr << ")";
			throw bad_message(oss.str());
		}

		std::ostringstream host_oss;
		host_oss << "http://" << hoststr;
		uri the_uri(host_oss.str());
		elem.m_uri = std::move(the_uri);

		m_via_list.push_back(std::move(elem));

		if (i >= len)
			break;

		if (istr[i] != ',')
			skip_comments(istr, i, len);
	}
}

via_list_value::via_list_value(const std::string &istr)
	: header_field_value(istr)
{
	parse(istr);
}

via_list_value::via_list_value(const via &v)
{
	m_via_list.push_back(v);
}

via_list_value::via_list_value(const std::vector<via> &vvec)
{
	for (auto v : vvec)
		m_via_list.push_back(v);
}

via_list_value &
via_list_value::operator+=(const via_list_value &vlist)
{
	if (!vlist.raw().empty()) {
		m_raw += ", ";
		m_raw += vlist.raw();
	}

	for (auto v : vlist.get())
		m_via_list.push_back(v);

	return *this;
}

std::string
via_list_value::str() const
{
	std::ostringstream oss;
	bool first = true;

	for (auto v : m_via_list) {
		if (!first)
			oss << ", ";

		oss << v.m_ver.str() << " ";
		if (v.m_uri.get_host().is_present()) {
			oss << v.m_uri.get_host().get();
			if (v.m_uri.get_port().is_present())
				oss << ":" << v.m_uri.get_port().numeric_port();
		}

		first = false;
	}

	return oss.str();
}

} // namespace http
} // namespace snf
