#include "version.h"
#include "status.h"
#include <cctype>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr size_t HTTP_VERSION_LENGTH = 8;
constexpr size_t HTTP_ABBR_VERSION_LENGTH = 3;

/*
 * Initialize HTTP version with the version
 * string of format: HTTP/<major>.<minor>.
 * Abbreviated version format skips the
 * protocol. It is simply <major>.<minor>.
 *
 * @param [in] vstr       - version string.
 * @param [in] allow_abbr - allow abbreviated
 *                          version string.
 *
 * @throws snf::http::bad_message exception if the
 * version string is not rightly formatted.
 */
version::version(const std::string &vstr, bool allow_abbr)
{
	std::ostringstream oss;
	oss << "invalid HTTP version (" << vstr << ")";

	if (allow_abbr) {
		if (vstr.size() < HTTP_ABBR_VERSION_LENGTH)
			throw bad_message(oss.str());
	} else {
		if (vstr.size() < HTTP_VERSION_LENGTH)
			throw bad_message(oss.str());
	}

	size_t idx = 0;

	if (vstr.compare(idx, 5, "HTTP/") == 0)
		idx += 5;
	else if (!allow_abbr)
		throw bad_message(oss.str());

	if (!std::isdigit(vstr[idx]))
		throw bad_message(oss.str());
	else
		m_major = static_cast<int>(vstr[idx] - '0');

	idx++;

	if (vstr[idx] != '.')
		throw bad_message(oss.str());

	idx++;

	if (!std::isdigit(vstr[idx]))
		throw bad_message(oss.str());
	else
		m_minor = static_cast<int>(vstr[idx] - '0');
}

} // namespace http
} // namespace snf
