#ifndef _SNF_HTTP_CMN_VERSION_H_
#define _SNF_HTTP_CMN_VERSION_H_

#include <string>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

/*
 * HTTP version.
 */
struct version
{
	int m_major;
	int m_minor;

	version()
		: m_major(1)
		, m_minor(1)
	{
	}

	version(int major, int minor)
		: m_major(major)
		, m_minor(minor)
	{
	}

	version(const version &v)
		: m_major(v.m_major)
		, m_minor(v.m_minor)
	{
	}

	version(const std::string &, bool allow_abbr = false);

	std::string str() const
	{
		std::ostringstream oss;
		oss << m_major << "." << m_minor;
		return oss.str();
	}
};

inline std::ostream &
operator<<(std::ostream &os, const version &v)
{
	os << "HTTP/" << v.str();
	return os;
}

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_VERSION_H_
