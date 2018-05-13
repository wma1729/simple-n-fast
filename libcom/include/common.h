#ifndef _SNF_COMMON_H_
#define _SNF_COMMON_H_

#include <sys/types.h>
#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <string>
#include "dbg.h"

#if defined(_WIN32)

	#include <Windows.h>
	#include <io.h>

	#define PATH_SEP        '\\'
	#define GET_ERRNO       static_cast<int>(GetLastError())
	#define SET_ERRNO(E)    SetLastError(E)
	#define strcasecmp      _stricmp
	#define strncasecmp     _strnicmp

	using mode_t = int;
	using pid_t = DWORD;
	using tid_t = DWORD;

	#define getpid()    GetCurrentProcessId()
	#define gettid()    GetCurrentThreadId()

#else /* if !defined(_WIN32) */

	#include <unistd.h>
	#include <sys/syscall.h>
	#include <errno.h>

	#define PATH_SEP                '/'
	#define INVALID_HANDLE_VALUE    (-1)
	#define GET_ERRNO               errno
	#define SET_ERRNO(E)            do { errno = (E); } while (0)

	using tid_t = uint32_t;

	#define gettid()    (tid_t)syscall(SYS_gettid)
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN      1023
#endif

#if !defined(ERRSTRLEN)
#define ERRSTRLEN       255
#endif

constexpr bool isnewline(int c) { return ((c == '\n') || (c == '\r')); }

namespace snf {

const char *syserr(char *, size_t, int);
const char *basename(char *, size_t, const char *, bool stripExt = false);
std::string trim(const std::string &);

} // namespace snf

#endif // _SNF_COMMON_H_
