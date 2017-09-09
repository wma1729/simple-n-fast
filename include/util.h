#ifndef _SNF_UTIL_H_
#define _SNF_UTIL_H_

#include "lock.h"
#include "error.h"

/**
 * Logging severity.
 */
typedef enum { INF, DBG, WRN, ERR } log_level_t;

/**
 * Log level to string representation.
 */
inline const char *
LevelStr(log_level_t l)
{
	switch (l)
	{
		case INF: return "INF";
		case DBG: return "DBG";
		case WRN: return "WRN";
		case ERR: return "ERR";
		default:  return "UNK";
	}
}

void Log(log_level_t, const char *, const char *, ...);
void Log(log_level_t, const char *, int, const char *, ...);
void Assert(bool, const char *, int, const char *, ...);
void Assert(bool, const char *, int, int, const char *, ...);

/**
 * Mutex Guard. The constructor acquires the mutex. The
 * destructor releases the mutex.
 */
class MutexGuard
{
private:
	Mutex	&mutex;

public:
	/**
	 * Acquires the 'mtx' mutex.
	 */
	MutexGuard(Mutex &mtx)
		: mutex(mtx)
	{
		int oserr;
		int ls = mutex.lock(&oserr);
		Assert((ls == E_ok), __FILE__, __LINE__, oserr,
			"failed to acquire mutex");
	}

	/**
	 * Releases the mutex.
	 */
	~MutexGuard()
	{
		int oserr;
		int ls = mutex.unlock(&oserr);
		Assert((ls == E_ok), __FILE__, __LINE__, oserr,
			"failed to release mutex");
	}
};

#endif // _SNF_UTIL_H_
