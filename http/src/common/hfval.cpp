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

		strvec.push_back(s);

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

std::vector<token>
token_list_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();
	std::vector<token> tokens;

	while (i < len) {
		token t;

		t.name = std::move(parse_token(istr, i, len));

		if (t.name.empty()) {
			std::ostringstream oss;
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		skip_spaces(istr, i, len);

		if ((i < len) && (istr[i] == ';'))
			t.parameters = std::move(parse_parameter(istr, i, len));

		tokens.push_back(t);

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

std::string
token_list_value::str() const
{
	std::ostringstream oss;
	bool first = true;

	for (auto t : m_tokens) {
		if (!first)
			oss << ", ";

		oss << t.name;
		for (auto p : t.parameters) {
			oss << ";" << p.first;
			if (!p.second.empty())
				oss << "=" << p.second;
		}

		first = false;
	}

	return oss.str();
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

void
host_value::parse(const std::string &hoststr)
{
	try {
		std::ostringstream oss;
		oss << "http://" << hoststr;
		std::string uristr = oss.str();

		snf::http::uri the_uri(uristr);

		if (the_uri.get_host().is_present()) {
			m_host = the_uri.get_host().get();
		} else {
			oss.str("");
			oss << "invalid host string specified: " << hoststr;
			throw bad_message(oss.str());
		}

		if (the_uri.get_port().is_present()) {
			m_port = the_uri.get_port().numeric_port();
		}
	} catch (snf::http::bad_uri &ex) {
		throw bad_message(ex.what());
	}
}

host_value::host_value(const std::string &hoststr)
	: header_field_value(hoststr)
	, m_port(0)
{
	parse(hoststr);
}

host_value::host_value(const std::string &hoststr, in_port_t port)
	: header_field_value(hoststr)
{
	parse(hoststr);
	if (port) {
		m_raw += ":";
		m_raw += std::to_string(port);
		m_port = port;
	}
}

std::string
host_value::str() const
{
	std::ostringstream oss;
	oss << m_host;
	if (m_port)
		oss << ":" << m_port;
	return oss.str();
}

void
media_type_value::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();

	m_mt.type = std::move(parse_token(istr, i, len));

	if (m_mt.type.empty())
		throw bad_message("no type found");
	else if (istr[i] != '/')
		throw bad_message("type is not followed by /");
	else
		i++;

	m_mt.subtype = std::move(parse_token(istr, i, len));

	if (m_mt.subtype.empty())
		throw bad_message("no subtype found");
	else if ((i < len) && ((istr[i] == ';') || is_whitespace(istr[i])))
		m_mt.parameters = std::move(parse_parameter(istr, i, len));
	else
		throw bad_message("invalid character after subtype");
}

media_type_value::media_type_value(const std::string &istr)
	: header_field_value(istr)
{
	parse(istr);
}

media_type_value::media_type_value(const std::string &type, const std::string &subtype)
{
	m_mt.type = type;
	m_mt.subtype = subtype;
}

media_type_value::media_type_value(const media_type &mt)
{
	m_mt.type = mt.type;
	m_mt.subtype = mt.subtype;
	m_mt.parameters = mt.parameters;
}

media_type_value::media_type_value(media_type &&mt)
{
	m_mt.type = std::move(mt.type);
	m_mt.subtype = std::move(mt.subtype);
	m_mt.parameters = std::move(mt.parameters);
}

std::string
media_type_value::str() const
{
	std::ostringstream oss;

	oss << m_mt.type << "/" << m_mt.subtype;
	for (auto elem : m_mt.parameters) {
		oss << ";" << elem.first;
		if (!elem.second.empty())
			oss << "=" << elem.second;
	}

	return oss.str();
}

} // namespace http
} // namespace snf
