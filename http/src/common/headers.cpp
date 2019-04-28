#include "common.h"
#include "headers.h"
#include "status.h"
#include "charset.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

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
		throw http_exception("no header field name", status_code::BAD_REQUEST);

	if (i >= len) {
		oss << "no header field value for field name (" << name << ")";
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	if (istr[i] != ':') {
		oss << "header field name (" << name << ") does not terminate with :";
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
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
		oss << "invalid header field value (" << name << ")";
		throw http_exception(oss.str(), status_code::BAD_REQUEST);
	}

	add(name, snf::trim(value));
}

void
headers::add(const std::string &name, const std::string &value)
{
	std::map<std::string, std::string>::iterator I = m_headers.find(name);
	if (I == m_headers.end()) {
		m_headers.insert(std::make_pair(name, value));
	} else {
		I->second.append(", ");
		I->second.append(value);
	}
}

void
headers::update(const std::string &name, const std::string &value)
{
	std::map<std::string, std::string>::iterator I = m_headers.find(name);
	if (I == m_headers.end()) {
		m_headers.insert(std::make_pair(name, value));
	} else {
		I->second = value;
	}
}

bool
headers::is_set(const std::string &name) const
{
	headers_map_t::const_iterator it = m_headers.find(name);
	if (it != m_headers.end())
		return true;
	return false;
}

std::string
headers::get(const std::string &name) const
{
	std::string s;

	headers_map_t::const_iterator it = m_headers.find(name);
	if (it != m_headers.end()) {
		s = it->second;
	} else {
		std::ostringstream oss;
		oss << "header field name (" << name << ") not found";
		throw std::out_of_range(oss.str());
	}

	return s;
}

int64_t
headers::content_length() const
{
	std::string s = std::move(get(CONTENT_LENGTH));
	return std::stoll(s);
}

void
headers::content_length(int64_t length)
{
	update(CONTENT_LENGTH, std::to_string(length));
}

} // namespace http
} // namespace snf
