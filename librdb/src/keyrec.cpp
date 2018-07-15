#include "keyrec.h"
#include "logmgr.h"

#define LEFT_OF(N)      kp->kp_keys[N].kr_left
#define RIGHT_OF(N)     kp->kp_keys[N].kr_right
#define HEIGHT_OF(N)    kp->kp_keys[N].kr_height

/*
 * Gets the index of the free key record slot.
 *
 * @return +ve index of the free key record slot
 * on success, -1 on failure.
 */
short
KeyRecords::getFree() const
{
	key_rec_t   *krec = kp->kp_keys;
	short       idx = -1;

	for (int i = 0; i < numOfKeys; ++i) {
		if (krec[i].kr_flags == KEY_FREE) {
			idx = i;
			break;
		}
	}

	return idx;
}

/*
 * Gets a key record.
 * - Gets a free key record slot.
 * - Initializes the key record.
 * - Sets flags, keys, data offset from key info.
 * - Increments the valid key count in page.
 *
 * @param [in] ki - The key information.
 *
 * @return +ve key record index in the key page on
 * success, -1 on failure.
 */
short
KeyRecords::getKeyRecord(const key_info_t *ki)
{
	short       idx = getFree();
	key_rec_t   *krec;

	if (idx < 0) {
		ERROR_STRM("KeyRecords")
			<< "no free key records available"
			<< snf::log::record::endl;
		return idx;
	}

	krec = &(kp->kp_keys[idx]);

	InitKeyRecord(krec);

	krec->kr_flags = KEY_INUSE;
	krec->kr_klen = short(ki->ki_klen);
	memcpy(krec->kr_key, ki->ki_key, ki->ki_klen);
	krec->kr_voff = ki->ki_voff;
	kp->kp_vcount++;

	return idx;
}

/*
 * Frees the key record at the specified index in
 * the key page.
 * - Resets the key record at the specified index.
 * - Decrements the valid key count in page.
 *
 * @param [in] idx - The key record index in the key
 *                   page to free.
 */
void
KeyRecords::freeKeyRecord(short idx)
{
	ASSERT(((idx >= 0) && (idx < numOfKeys)), "KeyRecords", 0,
		"invalid key record offset");

	key_rec_t *krec = &(kp->kp_keys[idx]);
	InitKeyRecord(krec);
	kp->kp_vcount--;
}

/*
 * Gets the balance factor of the key record node
 * at the specified index.
 *
 * @param [in] root - Key record index of the node.
 *
 * @return balance factor [-2, 2]
 */
int
KeyRecords::balanceFactor(short root)
{
	int bf = 0;
	int lh, rh;

	if (root != -1) {
		lh = (LEFT_OF(root) != -1) ? HEIGHT_OF(LEFT_OF(root)) : 0;
		rh = (RIGHT_OF(root) != -1) ? HEIGHT_OF(RIGHT_OF(root)) : 0;
		bf = lh - rh;
	}

	return bf;
}

/*
 * Get the height of the key record node at the
 * specified index.
 *
 * @param [in] root - Key record index of the node.
 *
 * @return height of the key record node. It is
 * 1 + MAX(HEIGHT(left), HEIGHT(right)).
 */
int
KeyRecords::height(short root)
{
	int h = 0;
	int lh, rh;

	if (root != -1) {
		lh = (LEFT_OF(root) != -1) ? HEIGHT_OF(LEFT_OF(root)) : 0;
		rh = (RIGHT_OF(root) != -1) ? HEIGHT_OF(RIGHT_OF(root)) : 0;
		h = 1 + ((lh < rh) ? rh : lh);
		HEIGHT_OF(root) = char(h);
	}

	return h;
}

/*
 * Rotate Left.
 *
 * @param [in] root - Root of the sub tree to left rotate.
 *
 * @return the new root of the rotated sub tree.
 */
short
KeyRecords::rotateLeft(short root)
{
	short pivot = RIGHT_OF(root);
	RIGHT_OF(root) = LEFT_OF(pivot);
	LEFT_OF(pivot) = root;

	height(root);
	height(pivot);

	return pivot;
}

/*
 * Rotate Right.
 *
 * @param [in] root - Root of the sub tree to right rotate.
 *
 * @return the new root of the rotated sub tree.
 */
short
KeyRecords::rotateRight(short root)
{
	short pivot = LEFT_OF(root);
	LEFT_OF(root) = RIGHT_OF(pivot);
	RIGHT_OF(pivot) = root;

	height(root);
	height(pivot);

	return pivot;
}

/*
 * Rotate Left Right
 *
 * @param [in] root - Root of the sub tree to rotate left right.
 *
 * @return the new root of the rotated sub tree.
 */
short
KeyRecords::rotateLeftRight(short root)
{
	LEFT_OF(root) = rotateLeft(LEFT_OF(root));
	return rotateRight(root);
}

/*
 * Rotate Right Left
 *
 * @param [in] root - Root of the sub tree to rotate right left.
 *
 * @return the new root of the rotated sub tree.
 */
short
KeyRecords::rotateRightLeft(short root)
{
	RIGHT_OF(root) = rotateRight(RIGHT_OF(root));
	return rotateLeft(root);
}

/*
 * Balances the tree by performing one or more rotations.
 *
 * @param [in] root - Root of the sub tree to balance.
 *
 * @return the new root of the balanced sub tree.
 */
short
KeyRecords::balanceTree(short root)
{
	int bf = balanceFactor(root);
	if (bf == -2) {
		bf = balanceFactor(RIGHT_OF(root));
		if (bf == -1) {
			root = rotateLeft(root);
		} else if (bf == 1) {
			root = rotateRightLeft(root);
		}
	} else if (bf == 2) {
		bf = balanceFactor(LEFT_OF(root));
		if (bf == -1) {
			root = rotateLeftRight(root);
		} else if (bf == 1) {
			root = rotateRight(root);
		}
	}

	return root;
}

/*
 * Puts/Adds key record in the given subtree.
 *
 * @param [in] root - Root of the sub tree to add
 *                    the key record.
 * @param [in] ki   - Key info containing key and
 *                    key data offset.
 * @param [out] idx - index of the key record where
 *                    the key is added.
 * @return the new root of the balanced sub tree.
 */
short
KeyRecords::put(short root, const key_info_t *ki, short *idx)
{
	if (root == -1) {
		root = getKeyRecord(ki);
		*idx = root;
	} else {
		const key_rec_t *krec = &(kp->kp_keys[root]);

		int cmp = keycmp(ki, krec);
		if (cmp < 0) {
			LEFT_OF(root) = put(LEFT_OF(root), ki, idx);
		} else if (cmp == 0) {
			kp->kp_keys[root].kr_voff = ki->ki_voff;
			*idx = root;
		} else /* if (cmp > 0) */ {
			RIGHT_OF(root) = put(RIGHT_OF(root), ki, idx);
		}

		height(root);
	}

	return balanceTree(root);
}

/*
 * Finds the mimimum key in the sub tree.
 *
 * @param [in] root - Root of the sub tree to find
 *                    the minimum key.
 * @return index of the minumum key record.
 */
short
KeyRecords::minKey(short root)
{
	if ((root != -1) && (LEFT_OF(root) != -1)) {
		return minKey(LEFT_OF(root));
	}

	return root;
}

/*
 * Finds the maximum key in the sub tree.
 *
 * @param [in] root - Root of the sub tree to find
 *                    the maximum key.
 * @return index of the maximum key record.
 */
short
KeyRecords::maxKey(short root)
{
	if ((root != -1) && (RIGHT_OF(root) != -1)) {
		return maxKey(RIGHT_OF(root));
	}

	return root;
}

/*
 * Removes the minimum key in the sub tree.
 *
 * @param [in] root - Root of the sub tree to remove
 *                    the minimum key.
 * @param [out] idx - Index of the key record where
 *                    the key is removed.
 *
 * @return the new root of the tree after the
 * minimum key is removed.
 */
short
KeyRecords::removeMin(short root, short *idx)
{
	if (root != -1) {
		if (LEFT_OF(root) == -1) {
			*idx = root;
			root = RIGHT_OF(root);
			freeKeyRecord(*idx);
		} else {
			LEFT_OF(root) = removeMin(LEFT_OF(root), idx);
		}
	}

	height(root);

	return balanceTree(root);
}

/*
 * Removes the maximum key in the sub tree.
 *
 * @param [in] root - Root of the sub tree to remove
 *                    the maximum key.
 * @param [out] idx - Index of the key record where
 *                    the key is removed.
 *
 * @return the new root of the tree after the
 * maximum key is removed.
 */
short
KeyRecords::removeMax(short root, short *idx)
{
	if (root != -1) {
		if (RIGHT_OF(root) == -1) {
			*idx = root;
			root = LEFT_OF(root);
			freeKeyRecord(*idx);
		} else {
			RIGHT_OF(root) = removeMax(RIGHT_OF(root), idx);
		}
	}

	height(root);

	return balanceTree(root);
}

/*
 * Removes key record from the given subtree.
 *
 * @param [in] root - Root of the sub tree to remove
 *                    the key record.
 * @param [in] ki   - Key info containing key.
 * @param [out] idx - Index of the key record where
 *                    the key is removed.
 * @return the new root of the balanced sub tree.
 */
short
KeyRecords::remove(short root, const key_info_t *ki, short *idx)
{
	if (root != -1) {
		key_rec_t *krec = &(kp->kp_keys[root]);

		int cmp = keycmp(ki, krec);
		if (cmp < 0) {
			LEFT_OF(root) = remove(LEFT_OF(root), ki, idx);
		} else if (cmp == 0) {
			if (LEFT_OF(root) == -1) {
				*idx = root;
				root = RIGHT_OF(root);
				freeKeyRecord(*idx);
			} else if (RIGHT_OF(root) == -1) {
				*idx = root;
				root = LEFT_OF(root);
				freeKeyRecord(*idx);
			} else {
				short maxInLeftSubTree = maxKey(LEFT_OF(root));
				memcpy(krec->kr_key, kp->kp_keys[maxInLeftSubTree].kr_key,
					kp->kp_keys[maxInLeftSubTree].kr_klen);
				krec->kr_klen = kp->kp_keys[maxInLeftSubTree].kr_klen;
				krec->kr_voff = kp->kp_keys[maxInLeftSubTree].kr_voff;
				LEFT_OF(root) = removeMax(LEFT_OF(root), idx);
			}
		} else /* if (cmp > 0) */ {
			RIGHT_OF(root) = remove(RIGHT_OF(root), ki, idx);
		}

		height(root);
	}

	return balanceTree(root);
}

/**
 * Compares key. memcmp() with key length into consideration.
 */
int
KeyRecords::keycmp(const key_info_t *ki, const key_rec_t *krec)
{
	int cmp = ki->ki_klen - krec->kr_klen;
	if (cmp == 0)
		cmp = memcmp(ki->ki_key, krec->kr_key, ki->ki_klen);
	return cmp;
}

/**
 * Get the number of free key record slots
 * in the key page.
 * @return number of free key record slots.
 */
int
KeyRecords::getFreeCount() const
{
	int count = 0;
	key_rec_t *krec = kp->kp_keys;

	for (int i = 0; i < numOfKeys; ++i) {
		if (krec[i].kr_flags == KEY_FREE) {
			count++;
		}
	}

	return count;
}

/**
 * Get the index of the key record containing
 * the key.
 *
 * @param [in] ki - Key info containing key.
 *
 * @return +ve index of the key record,
 * -1 if the key is not found.
 */
short
KeyRecords::get(const key_info_t *ki)
{
	short root = kp->kp_root;

	while (root != -1) {
		const key_rec_t *krec = &(kp->kp_keys[root]);

		int cmp = keycmp(ki, krec);
		if (cmp < 0) {
			root = LEFT_OF(root);
		} else if (cmp == 0) {
			break;
		} else /* if (cmp > 0) */ {
			root = RIGHT_OF(root);
		}
	}

	return root;
}

/**
 * Puts/Adds the key to the key page. Before
 * calling it make sure that there is free
 * space available in the key page by calling
 * getFreeCount().
 *
 * @param [in] ki - Key info containing key and
 *                  key data offset.
 * @return +ve index of the key record where the
 * key is inserted or -1 if the key is not inserted. 
 */
short
KeyRecords::put(const key_info_t *ki)
{
	short idx = -1;
	kp->kp_root = put(kp->kp_root, ki, &idx);
	return idx;
}

/**
 * Removes the key from the key page.
 *
 * @param [in] ki - Key info containing key.
 *
 * @return +ve index of the key record just
 * deleted or -1 if the key is not deleted.
 */
short
KeyRecords::remove(const key_info_t *ki)
{
	short idx = -1;
	kp->kp_root = remove(kp->kp_root, ki, &idx);
	return idx;
}
