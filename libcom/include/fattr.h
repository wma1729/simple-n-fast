#ifndef _SNF_FATTR_H_
#define _SNF_FATTR_H_

#include "common.h"
#if !defined(_WIN32)
	#include <sys/stat.h>
#endif

namespace snf {

/*
 * Different file types.
 */
enum class file_type
{
	unknown,    // Unknown file 
	regular,    // Regular file
	directory,  // Directory
	symlink,    // Symbolic link
	hardlink,   // Hard link
	fifo,       // Named pipe: Unix only
	character,  // Character device
	block,      // Block device: Unix only
	socket,     // Socket (stream-based): Unix only
	mntpoint    // Mount point: Windows only
};

/**
 * File type to string representation.
 */
constexpr const char *
file_type_string(file_type ft)
{
	switch (ft)
	{
		case file_type::regular:   return "REG";
		case file_type::directory: return "DIR";
		case file_type::symlink:   return "SLINK";
		case file_type::hardlink:  return "HLINK";
		case file_type::fifo:      return "FIFO";
		case file_type::character: return "CHAR";
		case file_type::block:     return "BLOCK";
		case file_type::socket:    return "SOCK";
		case file_type::mntpoint:  return "MNTPNT";
		default:                   return "UNK";
	}
}

/*
 * File attributes
 */
class file_attr
{
private:
	void init(const std::string &);
	void init(fhandle_t);

#if defined(_WIN32)
	uint64_t combine(DWORD hi, DWORD lo)
	{
		LARGE_INTEGER dummy;
		dummy.HighPart = hi;
		dummy.LowPart = lo;
		return dummy.QuadPart;
	}

	void init(const WIN32_FIND_DATAW &);
	void init(const WIN32_FILE_ATTRIBUTE_DATA &);
	void init(const BY_HANDLE_FILE_INFORMATION &, DWORD);
#else // !_WIN32
	void init(const struct stat &);
#endif

public:
	std::string     f_name;
	file_type       f_type;     // File type

	/**
	 * File mode. On Unix platforms, it is the file mode (protection).
	 * On Windows, it is the file attributes. Check GetFileAttributes() API.
	 */
	mode_t          f_mode;

	uint64_t        f_nlinks;   // Number of hard links to the file
	int64_t         f_size;     // File size in bytes
	uint64_t        f_inode;    // File inode
	int64_t         f_ctime;    // File create time
	int64_t         f_atime;    // File access time
	int64_t         f_mtime;    // File modification time
	int64_t         f_blksize;  // Block size for file system I/O
	int64_t         f_blkcnt;   // Number of blocks used by file

	/**
	 * ID of the device containing the file.  On Windows, it is the serial
	 * number of the volume containing the file.
	 */
	int64_t         f_dev;

	/**
	 * It holds the device ID for the special files.
	 */
	int64_t         f_rdev;

	uint32_t        f_uid;      // User ID
	uint32_t        f_gid;      // Group ID

	file_attr(const std::string &path, const std::string &name)
	{
		std::ostringstream oss;
		oss << path << snf::pathsep() << name;
		init(oss.str());
	}

	file_attr(const std::string &path)
	{
		init(path);
	}

	file_attr(fhandle_t hdl)
	{
		init(hdl);
	}

#if defined(_WIN32)

	file_attr(const WIN32_FIND_DATAW &fd)
	{
		init(fd);
	}

#else // !_WIN32

	file_attr(const struct stat &st)
	{
		init(st);
	}

#endif

};

} // namespace snf

#endif /* _SNF_FATTR_H_ */
