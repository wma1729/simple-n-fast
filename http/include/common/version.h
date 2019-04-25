#ifndef _SNF_HTTP_VERSION_H_
#define _SNF_HTTP_VERSION_H_

#include <string>

namespace snf {
namespace http {

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

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_VERSION_H_
