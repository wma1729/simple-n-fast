#ifndef _SNF_HTTP_CMN_VERSION_H_
#define _SNF_HTTP_CMN_VERSION_H_

#include <string>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr int MAJOR_VERSION = 1;
constexpr int MINOR_VERSION = 1;

/*
 * HTTP version.
 */
struct version
{
	static const std::string prefix;

	int m_major;
	int m_minor;

	version() : m_major(MAJOR_VERSION), m_minor(MINOR_VERSION) {}
	version(int major, int minor) : m_major(major), m_minor(minor) {}
	version(const std::string &, bool allow_abbr = false);

	std::string str() const noexcept
	{
		std::ostringstream oss;
		oss << m_major << "." << m_minor;
		return oss.str();
	}

	friend std::ostream & operator<<(std::ostream &os, const version &v) noexcept
	{
		os << prefix << v.str();
		return os;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_VERSION_H_
