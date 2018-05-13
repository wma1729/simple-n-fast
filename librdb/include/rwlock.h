#ifndef _SNF_RDB_RWLOCK_H_
#define _SNF_RDB_RWLOCK_H_

#if defined(_WIN32)
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <list>

/**
 * Read-write lock. Use Slim Read-Write Locks on
 * Windows and pthread_rwlock_t on Unix platforms.
 */
class RWLock
{
private:
	int                 cnt;

#if defined(_WIN32)
	SRWLOCK             lock;
#else
	pthread_rwlock_t    lock;
	int                 unlock(int *);
#endif

public:
	RWLock();
	~RWLock();

	/**
	 * Gets the reader/writer count.
	 */
	int getLockCount() const
	{
		return cnt;
	}

	int rdlock(int *oserr = 0);
	int tryrdlock(int *oserr = 0);
	int rdunlock(int *oserr = 0);
	int wrlock(int *oserr = 0);
	int trywrlock(int *oserr = 0);
	int wrunlock(int *oserr = 0);
};

/**
 * A pool of read-write lock. The lock pool is not
 * mutex protected. So it is the caller's responsibility
 * to make sure only 1 thread is accessing the pool
 * at a given time. This is intentional.
 */
class RWLockPool
{
private:
	std::list<RWLock *> pool;

public:
	/**
	 * Constructs the read-write lock pool. There are
	 * no locks to start with in the pool.
	 */
	RWLockPool()
	{
	}

	/**
	 * Destroys the read-write lock pool. All the locks
	 * in the pool are deleted.
	 */
	~RWLockPool()
	{
		std::list<RWLock *>::iterator itr;
		for (itr = pool.begin(); itr != pool.end(); ++itr) {
			RWLock *rwlock = *itr;
			delete rwlock;
		}
		pool.clear();
	}

	RWLock *get();
	void put(RWLock *);
};

#endif // _SNF_RDB_RWLOCK_H_
