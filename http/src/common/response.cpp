#include "common.h"
#include "response.h"
#include "charset.h"

namespace snf {
namespace http {

response_builder &
response_builder::response_line(const std::string &istr)
{
	size_t i;
	std::string vstr;
	std::string sstr;
	std::string rstr;
	size_t len = istr.size();
	std::ostringstream oss;

	if ((istr[len - 1] == '\n') || (istr[len - 2] == '\r'))
		len -= 2;

	for (i = 0; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		vstr.push_back(istr[i]);
	}

	if (vstr.empty())
		throw std::runtime_error("empty version");

	if (i >= len) {
		oss << "no status code after version (" << vstr << ")";
		throw std::runtime_error(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after version (" << vstr << ")";
		throw std::runtime_error(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (istr[i] == ' ')
			break;
		if (!std::isdigit(istr[i])) {
			oss << "unexpected character (" << istr[i] << ") in status code";
			throw std::runtime_error("empty status code");
		}

		sstr.push_back(istr[i]);
	}

	if (sstr.empty())
		throw std::runtime_error("empty status code");

	if (i >= len) {
		oss << "no reason phrase after " << vstr << " " << sstr;
		throw std::runtime_error(oss.str());
	}

	if (istr[i] != ' ') {
		oss << "no space after " << vstr << " " << sstr;
		throw std::runtime_error(oss.str());
	}

	if (sstr.size() != 3) {
		oss << "invalid status code (" << sstr << ")";
		throw std::runtime_error(oss.str());
	}

	for (i = i + 1; i < len; ++i) {
		if (is_whitespace(istr[i]) || is_vchar(istr[i]) || is_opaque(istr[i]))
			rstr.push_back(istr[i]);
		else
			break;
	}

	if (rstr.empty())
		throw std::runtime_error("empty reason phrase");

	if (i != len)
		throw std::runtime_error("unexpected character found in reason phrase");

	m_response.m_version = snf::http::version(vstr);
	m_response.m_status = static_cast<status_code>(std::stoi(sstr));
	m_response.m_reason = std::move(snf::trim(rstr));

	return *this;
}

response_builder &
response_builder::with_header(const std::string &name, const std::string &value)
{
	m_response.m_headers.add(name, value);
	return *this;
}

response_builder &
response_builder::header_line(const std::string &istr)
{
	m_response.m_headers.add(istr);
	return *this;
}

} // namespace http
} // namespace snf
