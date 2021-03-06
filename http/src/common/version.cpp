#include "version.h"
#include "status.h"
#include <cctype>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

const std::string http_protocol("HTTP");

/*
 * Initialize HTTP version with the version string of format:
 * - protocol/<major>.<minor>
 * - <major>.<minor>
 *
 * @param [in] vstr       - version string.
 *
 * @throws snf::http::bad_message exception if the
 * version string is not rightly formatted.
 */
version::version(const std::string &vstr)
{
	std::ostringstream oss;
	oss << "invalid HTTP version (" << vstr << ")";

	size_t idx = 0;
	size_t idx_of_slash = vstr.find_first_of('/');

	if (idx_of_slash != std::string::npos) {
		m_protocol = vstr.substr(0, idx_of_slash);
		idx = idx_of_slash + 1;
	}

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
