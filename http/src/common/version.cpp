#include "version.h"
#include "status.h"
#include <cctype>
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
 * @throws snf::http::bad_message exception if the
 * version string is not rightly formatted.
 */
version::version(const std::string &vstr)
{
	std::ostringstream oss;
	oss << "invalid HTTP version (" << vstr << ")";

	if (vstr.size() < HTTP_VERSION_LENGTH)
		throw bad_message(oss.str());

	if (vstr.compare(0, 4, "HTTP/") != 0)
		throw bad_message(oss.str());

	if (!std::isdigit(vstr[5]))
		throw bad_message(oss.str());
	else
		m_major = static_cast<int>(vstr[5] - '0');

	if (vstr[6] != '.')
		throw bad_message(oss.str());

	if (!std::isdigit(vstr[7]))
		throw bad_message(oss.str());
	else
		m_minor = static_cast<int>(vstr[7] - '0');
}

} // namespace http
} // namespace snf
