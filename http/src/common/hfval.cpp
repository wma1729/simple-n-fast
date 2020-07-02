#include "hfval.h"
#include "uri.h"
#include "status.h"
#include "charset.h"
#include "scanner.h"

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
	bool next = !istr.empty();
	std::vector<std::string> strvec;
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		std::string s;

		if (!scn.read_token(s)) {
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		strvec.push_back(std::move(s));

		scn.read_opt_space();

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
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

	for (auto it = m_strings.begin(); it != m_strings.end(); ++it) {
		if (it != m_strings.begin())
			oss << ", ";
		oss << *it;
	}

	return oss.str();
}

std::vector<token>
token_list_value::parse(const std::string &istr)
{
	bool next = !istr.empty();
	std::vector<token> tokens;
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		token t;

		if (!scn.read_token(t.m_name)) {
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		scn.read_opt_space();

		if (scn.read_special(';')) {
			if (!scn.read_parameters(t.m_parameters))
				throw bad_message("no parameters found");
		}

		tokens.push_back(std::move(t));

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
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

	for (auto it = m_tokens.begin(); it != m_tokens.end(); ++it) {
		if (it != m_tokens.begin())
			oss << ", ";

		oss << it->m_name;
		for (auto p : it->m_parameters) {
			oss << ";" << p.first;
			if (!p.second.empty())
				oss << "=" << p.second;
		}
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
	std::ostringstream oss;
	scanner scn{istr};

	if (!scn.read_token(m_mt.m_type))
		throw bad_message("no media type found");

	if (!scn.read_special('/'))
		throw bad_message("media type is not followed by \'/\'");

	if (!scn.read_token(m_mt.m_subtype))
		throw bad_message("no media subtype found");

	scn.read_opt_space();

	if (scn.read_special(';')) {
		scn.read_opt_space();

		if (!scn.read_parameters(m_mt.m_parameters))
			throw bad_message("no parameters found");
	}
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
	bool next = !istr.empty();
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		via v;
		std::string proto, ver, u;

		scn.read_opt_space();

		if (!scn.read_token(proto))
			throw bad_message("no protocol/version found");

		if (scn.read_special('/')) {
			if (!scn.read_token(ver))
				throw bad_message("no version found");
		} else {
			ver = proto;
			proto.clear();
		}

		if (!scn.read_space()) {
			oss << "no space after (";
			if (proto.empty())
				oss << ver;
			else
				oss << proto << "/" << ver;
			oss << ")";
			throw bad_message(oss.str());
		}

		if (!scn.read_uri(u))
			throw bad_message("no URI found");
	
		oss.str("");
		oss << "http://" << u;
		uri the_uri{oss.str()};

		scn.read_space();

		v.m_proto = proto;
		v.m_ver = version{ver};
		v.m_uri = std::move(the_uri);

		scn.read_comments(v.m_comments);

		m_via_list.push_back(v);

		scn.read_opt_space();

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
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

	for (auto it = m_via_list.begin(); it != m_via_list.end(); ++it) {
		if (it != m_via_list.begin())
			oss << ", ";

		if (!it->m_proto.empty())
			oss << it->m_proto << "/";

		oss << it->m_ver.str() << " ";

		if (it->m_uri.get_host().is_present()) {
			oss << it->m_uri.get_host().get();
			if (it->m_uri.get_port().is_present())
				oss << ":" << it->m_uri.get_port().numeric_port();
		}

		if (!it->m_comments.empty())
			oss << " " << it->m_comments;
	}

	return oss.str();
}

date_value::date_value(const std::string &istr)
	: header_field_value(istr)
{
	try {
		m_dt = new snf::datetime(istr, snf::time_format::imf, true);
	} catch (const std::invalid_argument &ex) {
		throw bad_message(ex.what());
	}
}

date_value::date_value(time_t t)
{
	m_dt = new snf::datetime(t, snf::unit::second, true);
}

date_value::date_value(const snf::datetime &dt)
{
	m_dt = new snf::datetime(dt);
}

std::string
date_value::str() const
{
	return m_dt->str(snf::time_format::imf);
}

} // namespace http
} // namespace snf
