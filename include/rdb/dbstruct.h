#ifndef _SNF_RDB_DBSTRUCT_H_
#define _SNF_RDB_DBSTRUCT_H_

#include "common.h"
#include "util.h"

#ifndef KEY_PAGE_HDR_SIZE
#define KEY_PAGE_HDR_SIZE   64
#endif

#ifndef MIN_KEY_PAGE_SIZE
#define MIN_KEY_PAGE_SIZE   1024
#endif

#ifndef KEY_PAGE_SIZE
#define KEY_PAGE_SIZE       4096
#endif

#ifndef MAX_KEY_LENGTH
#define MAX_KEY_LENGTH      64
#endif

#ifndef MAX_VALUE_LENGTH
#define MAX_VALUE_LENGTH    176
#endif

/* DB attributes */
extern "C"
typedef struct dbattr
{
	int a_kpsize;   // key page size
	int a_htsize;   // hash table size
} dbattr_t;

/* 80 bytes Key record */
extern "C"
typedef struct key_rec
{
	char    kr_flags;               // Flags: KEY_FREE|KEY_INUSE
	char    kr_height;              // Height of balanced tree
	short   kr_left;                // Offset of left node
	short   kr_right;               // Offset of right node
	short   kr_klen;                // Key length
	char    kr_key[MAX_KEY_LENGTH]; // Key
	int64_t kr_voff;                // Value offset in file
} key_rec_t;

#define KEY_FREE    char(0)
#define KEY_INUSE   char(1)

inline void
InitKeyRecord(key_rec_t *kr)
{
	memset(kr, 0, sizeof(key_rec_t));
	kr->kr_height = 1;
	kr->kr_left = -1;
	kr->kr_right = -1;
	kr->kr_voff = -1L;
}

#define NUM_OF_KEYS_IN_PAGE(B)  int(((B) - KEY_PAGE_HDR_SIZE) / sizeof(key_rec_t))

/* 64 bytes header followed by N keys */
extern "C"
typedef struct key_page
{
	short       kp_flags;   // Flags: 0|KPAGE_DELETED
	short       kp_vcount;  // Valid key count
	short       kp_root;    // Root index of balanced BST
	short       kp_unused1;
	int         kp_hash;    // Key hash
	int         kp_unused2;
	int64_t     kp_poff;    // Offset of previous page
	int64_t     kp_noff;    // Offset of next page
	int64_t     kp_unused3;
	int64_t     kp_unused4;
	int64_t     kp_unused5;
	int64_t     kp_unused6;
	key_rec_t   kp_keys[1]; // Array of keys (stored as BST)
} key_page_t;

#define KPAGE_DELETED   0x0001

inline void
InitKeyPage(key_page_t *kp, int kpsize)
{
	memset(kp, 0, kpsize);
	kp->kp_root = -1;
	kp->kp_poff = -1L;
	kp->kp_noff = -1L;
}

inline bool
IsKeyPageDeleted(const key_page_t *kp)
{
	if (kp && ((kp->kp_flags & KPAGE_DELETED) == KPAGE_DELETED)) {
		return true;
	}
	return false;
}

struct cnode;

typedef struct key_page_node
{
	key_page_t              *kpn_kp;    // Key page
	int64_t                 kpn_kpoff;  // Key page offset
	struct cnode            *kpn_cnode; // Corresponding cached element
	struct key_page_node    *kpn_prev;
	struct key_page_node    *kpn_next;
} key_page_node_t;

typedef struct key_info {
	char            ki_key[MAX_KEY_LENGTH]; // Key
	int             ki_klen;                // Key length
	int             ki_hash;                // Key hash
	int64_t         ki_voff;                // Key value offset
	key_page_node_t *ki_kpn;                // Key page node
	key_page_t      *ki_lkp;                // Last key page
	int64_t         ki_lkpoff;              // Last key page offset
	short           ki_kidx;                // Key index in key page
} key_info_t;

inline void
SetKeyInfo(key_info_t *ki, const char *key, int klen, int hash)
{
	memcpy(ki->ki_key, key, klen);
	int r = MAX_KEY_LENGTH - klen;
	if (r > 0)
		memset(ki->ki_key + klen, 0, r);

	ki->ki_klen = klen;
	ki->ki_hash = hash;
	ki->ki_voff = -1L;
	ki->ki_kpn = 0;
	ki->ki_lkp = 0;
	ki->ki_lkpoff = -1L;
	ki->ki_kidx = -1;
}

inline void
ValidateKeyInfo(const key_info_t *ki, const char *file, int line)
{
	Assert((ki->ki_kpn != 0), file, line,
		"found the key but key page node is not set");
	Assert((ki->ki_kpn->kpn_kp != 0), file, line,
		"found the key but key page is not set");
	Assert((ki->ki_kpn->kpn_kpoff != -1), file, line,
		"found the key but key page offset is not set");
	Assert((ki->ki_kidx != -1), file, line,
		"found the key but key index in page is not set");
	Assert((ki->ki_voff != -1), file, line,
		"found the key but value page offset is not set");
}

/* 256 bytes value/data page */
extern "C"
typedef struct value_page
{
	short   vp_flags;                   // flags: VPAGE_DELETED
	short   vp_unused1;                 // not used yet
	int     vp_unused2;                 // not used yet
	int     vp_klen;                    // key length
	int     vp_vlen;                    // value length
	char    vp_key[MAX_KEY_LENGTH];     // key
	char    vp_value[MAX_VALUE_LENGTH]; // value
} value_page_t;

#define VPAGE_DELETED   0x0001

inline bool
IsValuePageDeleted(const value_page_t *vp)
{
	return (vp && ((vp->vp_flags & VPAGE_DELETED) == VPAGE_DELETED));
}

inline void
InitValuePage(value_page_t *vp, const char *key, int klen, const char *value, int vlen)
{
	memset(vp, 0, sizeof(value_page_t));
	memcpy(vp->vp_key, key, klen);
	vp->vp_klen = klen;
	memcpy(vp->vp_value, value, vlen);
	vp->vp_vlen = vlen;
}

#endif // _SNF_RDB_DBSTRUCT_H_
