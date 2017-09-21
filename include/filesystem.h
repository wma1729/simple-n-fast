#ifndef _SNF_FILESYSTEM_H_
#define _SNF_FILESYSTEM_H_

/**
 * File system level operations.
 */
class FileSystem
{
public:
	static int  getHome(char *, size_t);
	static bool exists(const char *, int *oserr = 0);
	static int  isAbsolutePath(const char *);
	static int  mkdir(const char *, mode_t, int *oserr = 0);
	static int  rename(const char *, const char *, int *oserr = 0);
};

#endif // _SNF_FILESYSTEM_H_
