#include "rdb/unwind.h"
#include "error.h"

/**
 * Executes the unwinding of the stack.
 */
void
UnwindStack::execute()
{
	int             retval;
	KeyFile         *kf;
	ValueFile       *vf;
	key_page_t      *kp;
	value_page_t    *vp;

	while (!stk.empty()) {
		retval = E_ok;
		kf = 0;
		vf = 0;
		kp = 0;
		vp = 0;

		unwind_block_t &blk = stk.top();

		Assert((blk.file != 0), __FILE__, __LINE__,
			"file manager is not set");

		kf = dynamic_cast<KeyFile *> (blk.file);
		if (kf == 0) {
			vf = dynamic_cast<ValueFile *> (blk.file);
		}

		Assert(((kf != 0) || (vf != 0)), __FILE__, __LINE__,
			"file manager is neither key or value file manager");

		switch (blk.op) {
			case WRITE_FLAGS:
				if (kf) {
					kp = static_cast<key_page_t *> (blk.page);
					retval = kf->writeFlags(blk.pageOff, kp, blk.flags);
				} else {
					if (blk.page)
						vp = static_cast<value_page_t *> (blk.page);
					retval = vf->writeFlags(blk.pageOff, vp, blk.flags);
				}
				break;

			case WRITE_NEXT_OFFSET:
				Assert((kf != 0), __FILE__, __LINE__,
					"WRITE_NEXT_BLOCK is valid only for key file manager");
				kp = static_cast<key_page_t *> (blk.page);
				retval = kf->writeNextOffset(blk.pageOff, kp, blk.offset);
				break;

			case WRITE_PREV_OFFSET:
				Assert((kf != 0), __FILE__, __LINE__,
					"WRITE_PREV_BLOCK is valid only for key file manager");
				kp = static_cast<key_page_t *> (blk.page);
				retval = kf->writePrevOffset(blk.pageOff, kp, blk.offset);
				break;

			case FREE_PAGE:
				if (kf) {
					retval = kf->freePage(blk.offset);
				} else {
					retval = vf->freePage(blk.offset);
				}
				break;

			default:
				retval = E_invalid_arg;
				break;
		}

		Assert((retval == E_ok), __FILE__, __LINE__,
			"failed to unwind operation");

		stk.pop();
	}
}

/*
 * Pushes the unwind operation (restoring flags) on the stack.
 */
void
UnwindStack::writeFlags(File *file, void *page, int64_t pageOff, int flags)
{
	unwind_block_t blk;

	blk.op = WRITE_FLAGS;
	blk.file = file;
	blk.page = page;
	blk.pageOff = pageOff;
	blk.offset = -1L;
	blk.flags = flags;

	stk.push(blk);
}

/*
 * Pushes the unwind operation (restoring next offset) on the stack.
 */
void
UnwindStack::writeNextOffset(File *file, void *page, int64_t pageOff, int64_t offset)
{
	unwind_block_t blk;

	blk.op = WRITE_NEXT_OFFSET;
	blk.file = file;
	blk.page = page;
	blk.pageOff = pageOff;
	blk.offset = offset;
	blk.flags = -1;

	stk.push(blk);
}

/*
 * Pushes the unwind operation (restoring previous offset) on the stack.
 */
void
UnwindStack::writePrevOffset(File *file, void *page, int64_t pageOff, int64_t offset)
{
	unwind_block_t blk;

	blk.op = WRITE_PREV_OFFSET;
	blk.file = file;
	blk.page = page;
	blk.pageOff = pageOff;
	blk.offset = offset;
	blk.flags = -1;

	stk.push(blk);
}

/*
 * Pushes the unwind operation (freeing page) on the stack.
 */
void
UnwindStack::freePage(File *file, int64_t offset)
{
	unwind_block_t blk;

	blk.op = FREE_PAGE;
	blk.file = file;
	blk.page = 0;
	blk.pageOff = -1L;
	blk.offset = offset;
	blk.flags = -1;

	stk.push(blk);
}

/*
 * Either executes or clear the unwind stack based on the
 * status code.
 */
void
UnwindStack::unwind(int status)
{
	if (status == E_ok) {
		clear();
	} else {
		execute();
	}
}
