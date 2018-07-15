#include "rwlock.h"
#include "logmgr.h"
#include "error.h"

/**
 * Constructs the read write lock object.
 */
RWLock::RWLock()
{
	cnt = 0;

#if defined(_WIN32)
	InitializeSRWLock(&lock);
#else
	int error = pthread_rwlock_init(&lock, 0);
	ASSERT((error == 0), "RWLock", error,
		"failed to initialize read-write lock");
#endif
}

/**
 * Destroys the read write lock object.
 */
RWLock::~RWLock()
{
	ASSERT((cnt == 0), "RWLock", 0,
		"someone is holding the lock");

#if defined(_WIN32)
	// Nothing to do here
#else
	int error = pthread_rwlock_destroy(&lock);
	ASSERT((error == 0), "RWLock", error,
		"failed to destroy read-write lock");
#endif
}

#if !defined(_WIN32)

/*
 * Unlocks the read write lock mutex.
 * POSIX does not distinguish between unlocking of
 * read or write locks. There is only a single call.
 */
int
RWLock::unlock(int *oserr)
{
	int error = pthread_rwlock_unlock(&lock);
	if (error != 0) {
		ERROR_STRM("RWLock", error)
			<< "failed to unlock read-write lock"
			<< snf::log::record::endl;
		return E_unlock_failed;
	}

	return E_ok;
}

#endif

/**
 * Gets read/share lock. The lock is granted if no other thread has been
 * granted the lock in the write/exclusive mode. If another thread has been
 * granted the lock in read/shared mode, the lock is still granted. There
 * can be multiple readers but only one writer.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
RWLock::rdlock(int *oserr)
{
	if (oserr) *oserr = 0;

#if defined(_WIN32)
	AcquireSRWLockShared(&lock);
#else
	int error = pthread_rwlock_rdlock(&lock);
	if (error != 0) {
		if (oserr) *oserr = error;
		ERROR_STRM("RWLock", error)
			<< "failed to lock in read mode"
			<< snf::log::record::endl;
		return E_lock_failed;
	}
#endif

	cnt++;

	return E_ok;
}

/**
 * Tries to get read/share lock. The lock is granted if no other thread has
 * been granted the lock in the write/exclusive mode. If another thread has
 * been granted the lock in the write/exclusive mode, the call returns
 * immediately with a special error code. There is no blocking. If another
 * thread has been granted the lock in read/shared mode, the lock is still
 * granted. There can be multiple readers but only one writer.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, E_try_again if the lock is already acuired by
 * a different thread in write/exclusive mode, and -ve error code on failure.
 */
int
RWLock::tryrdlock(int *oserr)
{
	if (oserr) *oserr = 0;

#if defined(_WIN32)
	if (!TryAcquireSRWLockShared(&lock)) {
		return E_try_again;
	}
#else
	int error = pthread_rwlock_tryrdlock(&lock);
	if (error != 0) {
		if (oserr) *oserr = error;
		if ((error == EBUSY) || (error == EAGAIN)) {
			DEBUG_STRM("RWLock")
				<< "failed to lock in read mode, try again"
				<< snf::log::record::endl;
			return E_try_again;
		} else {
			ERROR_STRM("RWLock", error)
				<< "failed to lock in read mode"
				<< snf::log::record::endl;
			return E_lock_failed;
		}
	}
#endif

	cnt++;

	return E_ok;
}

/**
 * Releases the read lock.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
RWLock::rdunlock(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (cnt > 0) {
		cnt--;

#if defined(_WIN32)
		ReleaseSRWLockShared(&lock);
#else
		retval = unlock(oserr);
		if (retval != E_ok)
			cnt++;
#endif

	}

	return retval;
}

/**
 * Gets write/exclusive lock. The lock is granted if no other thread has been
 * granted the lock in any mode. There can be multiple readers but only one
 * writer.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
RWLock::wrlock(int *oserr)
{
	if (oserr) *oserr = 0;

#if defined(_WIN32)
	AcquireSRWLockExclusive(&lock);
#else
	int error = pthread_rwlock_wrlock(&lock);
	if (error != 0) {
		ERROR_STRM("RWLock", error)
			<< "failed to lock in write mode"
			<< snf::log::record::endl;
		return E_lock_failed;
	}
#endif

	cnt++;

	return E_ok;
}

/**
 * Tries to get write/exclusive lock. The lock is granted if no other thread
 * has been granted the lock in any mode. If another thread has been granted
 * the lock in any mode. the call returns immediately with a special error
 * code. There is no blocking. There can be multiple readers but only one
 * writer.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, E_try_again if the lock is already acquired by
 * a different thread, and -ve error code on failure.
 */
int
RWLock::trywrlock(int *oserr)
{
	if (oserr) *oserr = 0;

#if defined(_WIN32)
	if (!TryAcquireSRWLockExclusive(&lock)) {
		return E_try_again;
	}
#else
	int error = pthread_rwlock_trywrlock(&lock);
	if (error != 0) {
		if ((error == EBUSY) || (error == EAGAIN)) {
			DEBUG_STRM("RWLock")
				<< "failed to lock in write mode, try again"
				<< snf::log::record::endl;
			return E_try_again;
		} else {
			ERROR_STRM("RWLock", error)
				<< "failed to lock in write mode"
				<< snf::log::record::endl;
			return E_lock_failed;
		}
	}
#endif

	cnt++;

	return E_ok;
}

/**
 * Releases the write lock.
 *
 * @param [out] oserr - OS error code in case of failure.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
RWLock::wrunlock(int *oserr)
{
	int retval = E_ok;

	if (oserr) *oserr = 0;

	if (cnt > 0) {
		cnt--;

#if defined(_WIN32)
		ReleaseSRWLockExclusive(&lock);
#else
		retval = unlock(oserr);
		if (retval != E_ok)
			cnt++;
#endif

	}

	return retval;
}

/**
 * Gets a read-write lock from the pool of locks. If there are no
 * free locks in the pool, a new one is created.
 *
 * @return a read-write lock.
 */
RWLock *
RWLockPool::get()
{
	RWLock *rwlock = 0;

	if (!pool.empty()) {
		rwlock = pool.front();
		pool.pop_front();
	} else {
		rwlock = DBG_NEW RWLock();
	}

	return rwlock;
}

/**
 * Puts the read-write lock back to the pool of locks.
 *
 * @param [in] rwlock - read-write lock to return back
 *                      to the lock pool.
 */
void
RWLockPool::put(RWLock *rwlock)
{
	pool.push_back(rwlock);
}
