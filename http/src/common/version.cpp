#include "version.h"
#include <cctype>
#include <stdexcept>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr size_t HTTP_VERSION_LENGTH = 8;

/*
 * Initialize HTTP version with the version
 * string of format: HTTP/<major>.<minor>.
 *
 * @param [in] vstr - version string.
 *
 * @throws std::invalid_argument exception if the
 * version string is not rightly formatted.
 */
version::version(const std::string &vstr)
{
	std::ostringstream oss;
	oss << "invalid HTTP version (" << vstr << ")";

	if (vstr.size() < HTTP_VERSION_LENGTH)
		throw std::invalid_argument(oss.str());

	if (vstr.compare(0, 4, "HTTP/") != 0)
		throw std::invalid_argument(oss.str());

	if (!std::isdigit(vstr[5]))
		throw std::invalid_argument(oss.str());
	else
		m_major = static_cast<int>(vstr[5] - '0');

	if (vstr[6] != '.')
		throw std::invalid_argument(oss.str());

	if (!std::isdigit(vstr[7]))
		throw std::invalid_argument(oss.str());
	else
		m_minor = static_cast<int>(vstr[7] - '0');
}

} // namespace http
} // namespace snf
