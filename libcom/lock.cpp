#include <assert.h>
#include <errno.h>
#include "lock.h"
#include "error.h"

/**
 * Creates the mutex object.
 */
Mutex::Mutex()
{
#if defined(_WIN32)
	InitializeCriticalSection(&mtx);
#else
	int error = pthread_mutex_init(&mtx, 0);
	assert(error == 0);
#endif
}

/**
 * Destroys the mutex object.
 */
Mutex::~Mutex()
{
#if defined(_WIN32)
	DeleteCriticalSection(&mtx);
#else
	int error = pthread_mutex_destroy(&mtx);
	assert(error == 0);
#endif
}

/**
 * Locks/Acquires the mutex. If the mutex is already locked
 * by another thread, this call will block until the mutex
 * is released by the other thread.
 *
 * @param [out] oserr - OS error.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Mutex::lock(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

#if defined(_WIN32)
	EnterCriticalSection(&mtx);
#else
	int error = pthread_mutex_lock(&mtx);
	if (error != 0) {
		if (oserr) *oserr = error;
		retval = E_lock_failed;
	}		
#endif

	return retval;
}

/**
 * Locks/Acquires the mutex. If the mutex is already locked
 * by another thread, this call will return immediately with
 * E_try_again.
 *
 * @param [out] oserr - OS error.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Mutex::trylock(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

#if defined(_WIN32)
	if (!TryEnterCriticalSection(&mtx)) {
		retval = E_try_again;
	}
#else
	int error = pthread_mutex_trylock(&mtx);
	if (error != 0) {
		if (oserr) *oserr = error;
		if ((error == EBUSY) || (error == EAGAIN)) {
			retval = E_try_again;
		} else {
			retval = E_lock_failed;
		}
	}
#endif

	return retval;
}

/**
 * Unlocks/Releases the mutex.
 *
 * @param [out] oserr - OS error.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Mutex::unlock(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

#if defined(_WIN32)
	LeaveCriticalSection(&mtx);
#else
	int error = pthread_mutex_unlock(&mtx);
	if (error != 0) {
		if (oserr) *oserr = error;
		retval = E_unlock_failed;
	}
#endif

	return retval;
}
