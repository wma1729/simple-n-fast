#include "common.h"
#include "headers.h"
#include "status.h"
#include "charset.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

std::ostream &
operator<<(std::ostream &os, const headers &hdrs)
{
	for (auto &hdr : hdrs.m_headers)
		os << hdr.first << ": " << hdr.second << "\r\n";
	return os;
}

hdr_vec_t::iterator
headers::find(const std::string &name)
{
	for (hdr_vec_t::iterator I = m_headers.begin(); I != m_headers.end(); ++I)
		if (snf::streq(name, I->first, true))
			return I;
	return m_headers.end();
}

hdr_vec_t::const_iterator
headers::find(const std::string &name) const
{
	for (hdr_vec_t::const_iterator I = m_headers.begin(); I != m_headers.end(); ++I)
		if (snf::streq(name, I->first, true))
			return I;
	return m_headers.end();
}

void
headers::add(const std::string &istr)
{
	size_t i;
	size_t len = istr.size();
	std::string name;
	std::string value;
	std::ostringstream oss;

	if ((istr[len - 1] == '\n') || (istr[len - 2] == '\r'))
		len -= 2;

	for (i = 0; i < len; ++i) {
		if (!is_tchar(istr[i]))
			break;
		name.push_back(istr[i]);
	}

	if (name.empty())
		throw bad_message("no header field name");

	if (i >= len) {
		oss << "no header field value for field name (" << name << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ':') {
		oss << "header field name (" << name << ") does not terminate with :";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i)
		if (!is_whitespace(istr[i]))
			break;

	bool dquoted = false;
	bool commented = false;
	bool escaped = false;

	for (; i < len; ++i) {
		if (escaped) {
			if (is_escaped(istr[i]))
				escaped = false;
			else
				break;
		} else if (dquoted) {
			if (istr[i] == '"')
				dquoted = false;
			else if (!is_quoted(istr[i]))
				break;
		} else if (commented) {
			if (istr[i] == ')')
				commented = false;
			else if (!is_commented(istr[i]))
				break;
		} else if (istr[i] == '\\') {
			if (dquoted || commented)
				escaped = true;
			else
				break;
		} else if (istr[i] == '"') {
			dquoted = true;
		} else if (istr[i] == '(') {
			commented = true;
		} else if (!is_tchar(istr[i])) {
			break;
		}

		if (!escaped)
			value.push_back(istr[i]);
	}

	if (i != len) {
		oss << "invalid header field value for (" << name << ")";
		throw bad_message(oss.str());
	}

	add(name, snf::trim(value));
}

void
headers::add(const std::string &name, const std::string &value)
{
	hdr_vec_t::iterator I = find(name);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, value));
	} else {
		I->second.append(", ");
		I->second.append(value);
	}
}

void
headers::update(const std::string &name, const std::string &value)
{
	hdr_vec_t::iterator I = find(name);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, value));
	} else {
		I->second = value;
	}
}

void
headers::remove(const std::string &name)
{
	hdr_vec_t::iterator I = find(name);
	if (I != m_headers.end())
		m_headers.erase(I);
}

bool
headers::is_set(const std::string &name) const
{
	return (m_headers.end() != find(name));
}

std::string
headers::get(const std::string &name) const
{
	std::string s;

	hdr_vec_t::const_iterator it = find(name);
	if (it != m_headers.end()) {
		s = it->second;
	} else {
		std::ostringstream oss;
		oss << "header field name (" << name << ") not found";
		throw std::out_of_range(oss.str());
	}

	return s;
}

size_t
headers::content_length() const
{
	std::string s = std::move(get(CONTENT_LENGTH));
	return std::stoll(s);
}

void
headers::content_length(size_t length)
{
	update(CONTENT_LENGTH, std::to_string(length));
}

std::string
headers::transfer_encoding() const
{
	return get(TRANSFER_ENCODING);
}

void
headers::transfer_encoding(const std::string &coding)
{
	if (snf::streq(coding, TRANSFER_ENCODING_CHUNKED, true))
		update(TRANSFER_ENCODING, TRANSFER_ENCODING_CHUNKED);
	else
		throw not_implemented("only chunked transfer encoding is supported");
}

bool
headers::is_message_chunked() const
{
	try {
		std::string s = std::move(transfer_encoding());
		if (snf::streq(s, TRANSFER_ENCODING_CHUNKED, true))
			return true;

		throw not_implemented("only chunked transfer encoding is supported");
	} catch (std::out_of_range &) {
		return false;
	}

	return false;
}

std::string
headers::te() const
{
	return get(TE);
}

void
headers::te(const std::string &coding)
{
	if (snf::streq(coding, "trailers", true))
		update(TE, "trailers");
	else
		throw not_implemented("only trailers transfer encoding is accepted");
}

bool
headers::is_trailer_included() const
{
	try {
		std::string s = std::move(te());
		if (snf::streq(s, "trailers", true))
			return true;

		throw not_implemented("only trailers transfer encoding is accepted");
	} catch (std::out_of_range &) {
		return false;
	}

	return false;
}

} // namespace http
} // namespace snf
