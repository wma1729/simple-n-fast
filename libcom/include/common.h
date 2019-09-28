#ifndef _SNF_COMMON_H_
#define _SNF_COMMON_H_

#include <sys/types.h>
#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <cctype>
#include <string>
#include <sstream>
#include "dbg.h"

#if defined(_WIN32)

#define NOMINMAX
	#include <Windows.h>
	#include <io.h>

	#define strcasecmp      _stricmp
	#define strncasecmp     _strnicmp

	using mode_t = DWORD;
	using pid_t = DWORD;
	using tid_t = DWORD;
	using fhandle_t = HANDLE;

#else /* if !defined(_WIN32) */

	#include <unistd.h>
	#include <sys/syscall.h>
	#include <errno.h>

	#define INVALID_HANDLE_VALUE    (-1)

	using tid_t = uint32_t;
	using fhandle_t = int;

#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN      1023
#endif

#if !defined(ERRSTRLEN)
#define ERRSTRLEN       255
#endif

#define STUPID_WINDOWS(ARG)     ARG

#define VA_ARGS_CNT_(_1, _2, _3, _4, _5, N, ...) N
#define VA_ARGS_CNT(...)            STUPID_WINDOWS(VA_ARGS_CNT_(__VA_ARGS__, 5, 4, 3, 2, 1))

#define EXPAND_MACRO_(MACRO, NARGS) MACRO ## NARGS
#define EXPAND_MACRO(MACRO, NARGS)  EXPAND_MACRO_(MACRO, NARGS)

#define CALL_MACRO(MACRO, ...)      EXPAND_MACRO(MACRO, VA_ARGS_CNT(__VA_ARGS__))(__VA_ARGS__)

namespace snf {

template<typename T>
constexpr bool
is_flag_set(T value, T flags)
{
	return ((value & flags) == flags);
}

constexpr bool
isnewline(int c)
{
	return ((c == '\n') || (c == '\r'));
}

constexpr char
pathsep(void)
{
#if defined(_WIN32)
	return '\\';
#else
	return '/';
#endif
}

template<typename T, typename S>
T narrow_cast(S v)
{
	auto r = static_cast<T>(v);
	if (static_cast<S>(r) != v) {
		throw std::runtime_error("narrowing the value causes data loss");
	}
	return r;
}

inline bool
streq(const std::string &s1, const std::string &s2, bool ic = true)
{
	if (s1.size() != s2.size())
		return false;

	if (!ic)
		return s1 == s2;

	return std::equal(s1.begin(), s1.end(), s2.begin(),
			[](char c1, char c2) { return std::toupper(c1) == std::toupper(c2); }
		);
}

inline bool
is_decimal_number(const std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i) {
		if (!std::isdigit(str[i]))
			return false;
	}
	return true;
}

inline int
system_error(void)
{
#if defined(_WIN32)
	return static_cast<int>(::GetLastError());
#else
	return errno;
#endif
}

inline void
system_error(int syserr)
{
#if defined(_WIN32)
	::SetLastError(syserr);
#else
	errno = syserr;
#endif
}

inline pid_t
getpid(void)
{
#if defined(_WIN32)
	return ::GetCurrentProcessId();
#else
	return ::getpid();
#endif
}

inline tid_t
gettid(void)
{
#if defined(_WIN32)
	return ::GetCurrentThreadId();
#else
	return static_cast<tid_t>(::syscall(SYS_gettid));
#endif
}

inline bool
is_big_endian()
{
	union {
		short s;
		char  c[sizeof(short)];
	} u;

	u.s = 0x0102;
	return ((u.c[0] == 1) && (u.c[1] == 2));
}

const char *syserr(char *, size_t, int);
const char *basename(char *, size_t, const char *, bool stripExt = false);
std::string trim(const std::string &);
std::string bin2hex(const uint8_t *, size_t);

} // namespace snf

#endif // _SNF_COMMON_H_
