#ifndef _SNF_COMMON_H_
#define _SNF_COMMON_H_

#include <sys/types.h>
#include <time.h>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdint.h>
#include <inttypes.h>
#include <string>

#if defined(_WIN32)

	#if !defined(WIN32_LEAN_AND_MEAN)
	#define WIN32_LEAN_AND_MEAN
	#endif

	#if defined(_DEBUG)
	#define _CRTDBG_MAP_ALLOC
	#include <cstdlib>
	#include <crtdbg.h>
	#define DBG_NEW         new (_NORMAL_BLOCK, __FILE__, __LINE__)
	#else
	#define DBG_NEW         new
	#endif

	#define PATH_SEP        '\\'

	#define GET_ERRNO       (int)GetLastError()
	#define SET_ERRNO(E)    SetLastError(E)
	#define strcasecmp      _stricmp
	#define strncasecmp     _strnicmp

	#include <Windows.h>
	#include <io.h>

	typedef int     mode_t;
	typedef HANDLE  fhandle_t;
	typedef DWORD   pid_t;
	typedef DWORD   tid_t;

#else /* if !defined(_WIN32) */

	#define _FILE_OFFSET_BITS 64

	#if !defined(_REENTRANT)
	#define _REENTRANT
	#endif

	#define PATH_SEP                '/'
	#define INVALID_HANDLE_VALUE    (-1)
	#define GET_ERRNO               errno
	#define SET_ERRNO(E)            do { errno = (E); } while (0)
	#define DBG_NEW                 new

	#include <sys/time.h>
	#include <sys/stat.h>
	#include <sys/syscall.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <errno.h>
	#include <cstdlib>

	typedef int             fhandle_t;
	typedef unsigned int    tid_t;

#endif

#if defined(_WIN32)
	#define getpid()    GetCurrentProcessId()
	#define gettid()    GetCurrentThreadId()
#elif defined(__linux__)
	#define gettid()    (tid_t)syscall(SYS_gettid)
#else
	#define gettid()    (tid_t)pthread_self()
#endif

#if !defined(MAXPATHLEN)
#define MAXPATHLEN      1023
#endif

#if !defined(ERRSTRLEN)
#define ERRSTRLEN       255
#endif

#define ISNEWLINE(C)    (((C) == '\n') || ((C) == '\r'))

/**
 * Local time. Hides platform variations.
 */
typedef struct
{
	int year;       // 4 digit year
	int month;      // month: 1 is January and 12 is December
	int day;        // day: 1 - 31
	int hour;       // hour: 0 - 23
	int minute;     // minute: 0 - 59
	int second;     // second: 0 - 59
	int msec;       // milli-second: 0 - 999
} local_time_t;

time_t     GetLocalTime(local_time_t *);
const char *LocalTimeToString(local_time_t *, char *, size_t);
const char *GetErrorStr(char *, size_t, int);
const char *GetBaseName(char *, size_t, const char *, bool stripExt = false);

#endif // _SNF_COMMON_H_
