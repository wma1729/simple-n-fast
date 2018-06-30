#ifndef _SNF_DIR_H_
#define _SNF_DIR_H_

#include "common.h"
#include <regex>
#include "fattr.h"
#if defined(_WIN32)
	#include <direct.h>
#else // !_WIN32
	#include <dirent.h>
#endif

namespace snf {

using file_visitor = void(const file_attr &);

class directory
{
private:
	std::string m_path;
	std::regex  m_pattern;

#if defined(_WIN32)
	HANDLE              m_hdl = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATAW    m_fd;
	bool                m_first_read = true;
#else // !_WIN32
	DIR                 *m_dir = nullptr;
#endif

public:
	directory(const std::string &, const std::string &pattern = { R"(.*)" });
	~directory();
	bool read(file_visitor);
};

} // namespace snf

#endif // _SNF_DIR_H_
