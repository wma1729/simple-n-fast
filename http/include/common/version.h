#ifndef _SNF_HTTP_CMN_VERSION_H_
#define _SNF_HTTP_CMN_VERSION_H_

#include <string>
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

constexpr int MAJOR_VERSION = 1;
constexpr int MINOR_VERSION = 1;

extern const std::string http_protocol;

/*
 * HTTP version.
 */
struct version
{
	std::string m_protocol;
	int         m_major;
	int         m_minor;

	version() : m_protocol(http_protocol), m_major(MAJOR_VERSION), m_minor(MINOR_VERSION) {}
	version(int major, int minor) : m_protocol(http_protocol), m_major(major), m_minor(minor) {}
	version(const std::string & proto, int major, int minor) : m_protocol(proto), m_major(major), m_minor(minor) {}
	version(const std::string &);

	const std::string & protocol() const
	{
		return m_protocol.empty() ? http_protocol : m_protocol;
	}

	std::string str() const noexcept
	{
		std::ostringstream oss;
		if (!m_protocol.empty())
			oss << m_protocol << "/";
		oss << m_major << "." << m_minor;
		return oss.str();
	}

	friend std::ostream & operator<<(std::ostream &os, const version &v) noexcept
	{
		os << v.str();
		return os;
	}
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_VERSION_H_
