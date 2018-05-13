#ifndef _SNF_FILESYSTEM_H_
#define _SNF_FILESYSTEM_H_

#include "common.h"

/**
 * File system level operations.
 */

namespace snf {
namespace fs {

int      get_home(char *, size_t);
bool     exists(const char *, int *oserr = 0);
int64_t  size(const char *, int *oserr = 0);
int      is_abs_path(const char *);
int      mkdir(const char *, mode_t, int *oserr = 0);
int      rename(const char *, const char *, int *oserr = 0);
int      remove_file(const char *, int *oserr = 0);
int      remove_dir(const char *, int *oserr = 0);

} // namespace fs
} // namespace snf

#endif // _SNF_FILESYSTEM_H_
