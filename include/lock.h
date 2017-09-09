#ifndef _SNF_LOCK_H_
#define _SNF_LOCK_H_

#if defined(WINDOWS)
#include <Windows.h>
#else
#include <pthread.h>
#endif

/**
 * A simple mutex. CRITICAL_SECTION is used on Windows
 * and POSIX mutex with default attributes on Unix
 * platforms.
 */
class Mutex
{
private:
#if defined(WINDOWS)
	CRITICAL_SECTION    mtx;
#else
	pthread_mutex_t     mtx;
#endif

public:
	Mutex();
	~Mutex();

	int lock(int *oserr = 0);
	int trylock(int *oserr = 0);
	int unlock(int *oserr = 0);
};

#endif // _SNF_LOCK_H_
