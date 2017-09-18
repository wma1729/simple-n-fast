#ifndef _CACHE_H_
#define _CACHE_H_

#include <list>
#include "util.h"
#include "rdb/dbfiles.h"
#include "rdb/pagemgr.h"

/* LRU cache node */
typedef struct cnode
{
	key_page_node_t *c_kpn;     /* key page node */
	struct cnode    *c_prev;    /* previous LRU node */
	struct cnode    *c_next;    /* next LRU node */
} cnode_t;

/**
 * LRU Cache. The cache is maintained as a doubly
 * linked list. The cached node is cnode_t. The
 * most recently used element is at the front of
 * the list and the least recently used elememt
 * is at the back of the list. When a new element
 * is added or updated, it is moved to the front
 * of the list. When the cache is full and a new
 * element is added, the last element is reused
 * and moved to the front of the list.
 */
class LRUCache
{
private:
	KeyFile *keyFile;
	PageMgr *pageMgr;
	int     kpSize;
	int     max;
	int     num;
	cnode_t *head;
	cnode_t *tail;
	Mutex   mutex;

	cnode_t *removeLast();
	cnode_t *getCacheNode();
	void add(cnode_t *);
	void free(cnode_t *);
	int getPage(key_page_t *&, int64_t offset = -1);

public:
	/**
	 * Constructs LRU Cache object.
	 *
	 * @param [in] keyFile  - Key file.
	 * @param [in] kpSize   - Key page size.
	 */
	LRUCache(KeyFile *keyFile, int kpSize)
		: keyFile(keyFile),
		  kpSize(kpSize),
		  num(0),
		  head(0),
		  tail(0),
		  mutex()
	{
		pageMgr = new PageMgr(kpSize);
		max = pageMgr->getNumberOfPages();
	}

	/**
	 * Destroys LRU Cache object.
	 */
	~LRUCache()
	{
		{
			MutexGuard guard(mutex);

			cnode_t *n;
			while ((n = head) != 0) {
				head = head->c_next;
				::free(n);
			}

			tail = 0;
			num = 0;
		}

		if (pageMgr) {
			delete pageMgr;
		}
	}

	int  get(key_page_node_t *&, int64_t offset = -1L);
	int  update(key_page_node_t *, int64_t offset = -1L);
	void touch(key_page_node_t *);
	void free(key_page_node_t *);
};

#endif // _CACHE_H
