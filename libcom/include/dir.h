#ifndef _SNF_DIR_H_
#define _SNF_DIR_H_

#include "common.h"
#include <iterator>
#include <memory>
#include <regex>
#include "fattr.h"
#if defined(_WIN32)
	#include <direct.h>
#else // !_WIN32
	#include <dirent.h>
#endif

namespace snf {

class dir_impl
{
private:
	friend class directory;

	std::string m_path;
	std::regex  m_pattern;
	file_attr   m_fa;

#if defined(_WIN32)
	HANDLE      m_hdl = INVALID_HANDLE_VALUE;
#else // !_WIN32
	DIR         *m_dir = nullptr;
#endif

	bool next();

public:
	dir_impl() = default;
	dir_impl(const std::string &path, const std::string &pattern);
	~dir_impl();
	operator const file_attr & () { return m_fa; }
};

class directory
{
private:
	std::shared_ptr<dir_impl> m_impl;

public:
	using iterator_category = std::input_iterator_tag;
	using value_type        = file_attr;
	using difference_type   = void;
	using pointer           = const file_attr *;
	using reference         = const file_attr &;

	directory()
		: m_impl(DBG_NEW dir_impl())
	{
	}

	directory(const std::string &path, const std::string &pattern = { R"(.*)" })
		: m_impl(DBG_NEW dir_impl(path, pattern))
	{
	}

	directory(const directory &) = default;
	directory(directory &&) = default;
	~directory() = default;

	directory & operator=(const directory &) = default;
	directory & operator=(directory &&) = default;

	// dereference
	reference operator*() const noexcept
	{
		return *m_impl;
	}

	// member
	pointer operator->() const noexcept
	{
		const file_attr &fa = *m_impl;
		return &fa;
	}

	// prefix
	directory & operator++()
	{
		m_impl->next();
		return *this;
	}

	// postfix
	directory & operator++(int)
	{
		m_impl->next();
		return *this;
	}

	friend bool operator==(const directory &, const directory &);
	friend bool operator!=(const directory &, const directory &);
};

inline bool operator==(const directory &lhs, const directory &rhs)
{
	return *(lhs.m_impl) == *(rhs.m_impl);
}

inline bool operator!=(const directory &lhs, const directory &rhs)
{
	return !(lhs == rhs);
}

inline directory begin(directory d) noexcept { return d; }
inline directory end(directory) noexcept { return directory(); }

} // namespace snf

#endif // _SNF_DIR_H_
