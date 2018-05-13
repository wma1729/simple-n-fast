#ifndef _SNF_RDB_KEYREC_H_
#define _SNF_RDB_KEYREC_H_

#include "dbstruct.h"

/**
 * Manages key records as balanced AVL binary tree.
 */
class KeyRecords
{
private:
	key_page_t  *kp;
	int         numOfKeys;

	short getFree() const;
	short getKeyRecord(const key_info_t *);
	void  freeKeyRecord(short);
	int   balanceFactor(short);
	int   height(short);
	short rotateLeft(short);
	short rotateRight(short);
	short rotateLeftRight(short);
	short rotateRightLeft(short);
	short balanceTree(short);
	short put(short, const key_info_t *, short *);
	short minKey(short);
	short maxKey(short);
	short removeMin(short, short *);
	short removeMax(short, short *);
	short remove(short, const key_info_t *, short *);
	int keycmp(const key_info_t *, const key_rec_t *);

public:
	/**
	 * Constructs the key records object.
	 *
	 * @param [in] kp     - Key page containing key records.
	 * @param [in] kpsize - Key page size.
	 */
	KeyRecords(key_page_t *kp, int kpsize)
		: kp(kp),
		  numOfKeys(NUM_OF_KEYS_IN_PAGE(kpsize))
	{
	}

	/**
	 * Destroys the key records object.
	 */
	~KeyRecords()
	{
	}

	int   getFreeCount() const;
	short get(const key_info_t *);
	short put(const key_info_t *);
	short remove(const key_info_t *);
};

#endif // _SNF_RDB_KEYREC_H_
