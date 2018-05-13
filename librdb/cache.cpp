#include "error.h"
#include "log.h"
#include "rdb/cache.h"

/*
 * Removes the last (or LRU) element from
 * the cache. If the element points to a valid
 * key page node which in turn points to a
 * valid key page, the key page is freed.
 *
 * @return the node just removed.
 */
cnode_t *
LRUCache::removeLast()
{
	std::lock_guard<std::mutex> guard(mutex);

	cnode_t *cn = tail;
	if (tail) {
		if (tail->c_prev) {
			tail->c_prev->c_next = 0;
			tail = tail->c_prev;
		} else {
			head = 0;
			tail = 0;
		}

		if (cn->c_kpn) {
			if (cn->c_kpn->kpn_kp) {
				pageMgr->free(cn->c_kpn->kpn_kp);
				cn->c_kpn->kpn_kp = 0;
			}
			cn->c_kpn->kpn_kpoff = -1L;
			cn->c_kpn->kpn_cnode = 0;
			cn->c_kpn = 0;
		}

		cn->c_prev = cn->c_next = 0;
		num--;
	}

	return cn;
}

/*
 * Gets a cache node.
 * If the cache is full, the last element is
 * removed, updated, and returned.
 *
 * @return cache node, NULL in case of error.
 */
cnode_t *
LRUCache::getCacheNode()
{
	cnode_t	*cn = 0;

	if (num >= max) {
		cn = removeLast();
	} else {			
		cn = (cnode_t *)calloc(1, sizeof(cnode_t));
	}

	return cn;
}

/*
 * Adds the cache node to the cache.
 *
 * @param [in] cn  - Cache node
 */
void
LRUCache::add(cnode_t *cn)
{
	std::lock_guard<std::mutex> guard(mutex);

	if (cn) {
		cn->c_prev = 0;
		cn->c_next = head;
		if (head) {
			head->c_prev = cn;
		} else {
			tail = cn;
		}
		head = cn;
		num++;
	}
}

/*
 * Frees the specified cache node.
 *
 * @param [in] cn - Cached node.
 */
void
LRUCache::free(cnode_t *cn)
{
	std::lock_guard<std::mutex> guard(mutex);

	if (cn->c_prev) {
		cn->c_prev->c_next = cn->c_next;
	} else {
		head = cn->c_next;
		if (head) {
			head->c_prev = 0;
		}
	}

	if (cn->c_next) {
		cn->c_next->c_prev = cn->c_prev;
	} else {
		tail = cn->c_prev;
		if (tail) {
			tail->c_next = 0;
		}
	}

	::free(cn);
}

/*
 * Get a page. If the offset is not -1, read the
 * page content from the key file.
 *
 * @param [inout] kp  - Key Page
 * @param [in] offset - Page offset in the key file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
LRUCache::getPage(key_page_t *&kp, int64_t offset)
{
	const char  *who = "LRUCache::getPage";
	int         retval = E_ok;

	kp = (key_page_t *)(pageMgr->get());
	if (kp == 0) {
		Log(ERR, who,
			"unable to get in-memory page");
		retval = E_no_memory;
	} else if (offset != -1L) {
		retval = keyFile->read(offset, kp, kpSize);
		if (retval != E_ok) {
			Log(ERR, who,
				"unable to read page at offset %" PRId64 " from %s",
				offset, keyFile->name());
			pageMgr->free(kp);
		}
	}

	return retval;
}

/*
 * Get the key page node. The key page is obtained
 * possibly by reading the page content from key
 * file if the offset is not -1.
 *
 * @param [inout] kpn - Key page node.
 * @param [in] offset - Page offset in the key file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
LRUCache::get(key_page_node_t *&kpn, int64_t offset)
{
	const char  *who = "LRUCache::get";
	int         retval;
	key_page_t  *kp = 0;
	cnode_t     *cn = 0;

	cn = getCacheNode();
	if (cn == 0) {
		Log(ERR, who,
			"failed to allocate memory for LRU cache node");
		return E_no_memory;
	}

	kpn = (key_page_node_t *)malloc(sizeof(key_page_node_t));
	if (kpn == 0) {
		Log(ERR, who,
			"failed to allocate memory for key page node");
		::free(cn);
		return E_no_memory;
	}

	retval = getPage(kp, offset);
	if (retval != E_ok) {
		::free(kpn);
		::free(cn);
	} else {
		kpn->kpn_kp = kp;
		kpn->kpn_kpoff = offset;
		kpn->kpn_cnode = cn;
		kpn->kpn_prev = kpn->kpn_next = 0;
		cn->c_kpn = kpn;
		add(cn);
	}

	return retval;
}

/**
 * Updates the cached key page node with new page
 * and offset. Also updates the cache i.e. move
 * the cached element to the top of the list.
 *
 * @param [in] kpn    - Key page node
 * @param [in] offset - Key page offset
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
LRUCache::update(key_page_node_t *kpn, int64_t offset)
{
	const char  *who = "LRUCache::update";
	int         retval = E_ok;
	key_page_t  *kp = 0;
	cnode_t     *cn = 0;

	cn = getCacheNode();
	if (cn == 0) {
		Log(ERR, who,
			"failed to allocate memory for LRU cache node");
		return E_no_memory;
	}

	retval = getPage(kp, offset);
	if (retval != E_ok) {
		::free(cn);
	} else {
		kpn->kpn_kp = kp;
		kpn->kpn_kpoff = offset;
		kpn->kpn_cnode = cn;
		kpn->kpn_prev = kpn->kpn_next = 0;
		cn->c_kpn = kpn;
		add(cn);
	}

	return retval;
}

/**
 * Touches the cached element i.e. move it to the
 * front of the list.
 *
 * @param [in] kpn - Key page node
 */
void
LRUCache::touch(key_page_node_t *kpn)
{
	std::lock_guard<std::mutex> guard(mutex);

	if (kpn == 0) {
		return;
	}

	if (kpn->kpn_kp == 0) {
		return;
	}

	if (kpn->kpn_cnode == 0) {
		return;
	}

	cnode_t *cn = (cnode_t *) (kpn->kpn_cnode);

	if (cn->c_prev) {
		cn->c_prev->c_next = cn->c_next;
	} else {
		// Already at the head; do nothing
		return;
	}

	if (cn->c_next) {
		cn->c_next->c_prev = cn->c_prev;
	}

	cn->c_prev = 0;
	cn->c_next = head;
	head->c_prev = cn;
	head = cn;

	return;
}

/**
 * Frees the cached node contained in the key page
 * node. If the node points to a valid key page node
 * which inturn points to a valid key page,
 * the page is freed.
 *
 * @param [in] kpn - Key page node
 */
void
LRUCache::free(key_page_node_t *kpn)
{
	cnode_t *cn = kpn->kpn_cnode;
	if (cn == 0)
		return;

	free(cn);

	if (kpn->kpn_kp) {
		pageMgr->free(kpn->kpn_kp);
		kpn->kpn_kp = 0;
	}
	kpn->kpn_kpoff = -1L;
	kpn->kpn_cnode = 0;

	::free(kpn);
}
