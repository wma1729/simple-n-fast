#include "version.h"
#include <cctype>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr size_t HTTP_VERSION_LENGTH = 8;

version::version(const std::string &vstr)
{
	std::ostringstream oss;
	oss << "invalid HTTP version (" << vstr << ")";

	if (vstr.size() < HTTP_VERSION_LENGTH)
		throw std::runtime_error(oss.str());

	std::string prefix = vstr.substr(0, 5);

	if (prefix != "HTTP/")
		throw std::runtime_error(oss.str());

	if (!std::isdigit(vstr[5]))
		throw std::runtime_error(oss.str());
	else
		m_major = static_cast<int>(vstr[5] - '0');

	if (vstr[6] != '.')
		throw std::runtime_error(oss.str());

	if (!std::isdigit(vstr[7]))
		throw std::runtime_error(oss.str());
	else
		m_minor = static_cast<int>(vstr[7] - '0');
}

} // namespace http
} // namespace snf
