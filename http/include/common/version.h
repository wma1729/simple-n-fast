#ifndef _SNF_HTTP_CMN_VERSION_H_
#define _SNF_HTTP_CMN_VERSION_H_

#include <string>
#include <ostream>

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

	version(const std::string &);
};

inline std::ostream &
operator<<(std::ostream &os, const version &v)
{
	os << "HTTP/" << v.m_major << "." << v.m_minor;
	return os;
}

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_VERSION_H_
