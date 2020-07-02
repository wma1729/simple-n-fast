#include "version.h"
#include "status.h"
#include <cctype>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr size_t VERSION_LONG_LEN = 8;
constexpr size_t VERSION_SHORT_LEN = 3;

const std::string version::prefix { "HTTP/" };

/*
 * Initialize HTTP version with the version string of format:
 * - HTTP/<major>.<minor>
 * - <major>.<minor>
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

	size_t idx = 0;

	switch (vstr.size()) {
		case VERSION_LONG_LEN:
			if (vstr.compare(idx, prefix.size(), prefix) == 0)
				idx += prefix.size();
			else
				throw bad_message(oss.str());
			break;

		case VERSION_SHORT_LEN:
			break;

		default:
			throw bad_message(oss.str());
			break;
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
