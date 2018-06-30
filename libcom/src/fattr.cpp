#include "fattr.h"
#if defined(_WIN32)
	#include "i18n.h"
	#include "timeutil.h"
#endif

namespace snf {

void
file_attr::init(const std::string &path)
{
#if defined(_WIN32)

	wchar_t *pathW = mbs2wcs(path.c_str());
	if (pathW == nullptr)
		throw std::runtime_error("invalid file path");
 
	WIN32_FILE_ATTRIBUTE_DATA fad;

	if (!GetFileAttributesExW(pathW, GetFileExInfoStandard, &fad)) {
		delete [] pathW;

		std::ostringstream oss;
		oss << "failed to get attributes of path " << path;
		throw std::system_error(
			snf::system_error(),
			std::system_category(),
			oss.str());
	}

	delete [] pathW;
	init(fad);

#else // !_WIN32

	struct stat st;
	if (stat(path.c_str(), &st) < 0) {
		std::ostringstream oss;
		oss << "failed to get attributes of path " << path;
		throw std::system_error(
			snf::system_error(),
			std::system_category(),
			oss.str());
	}

	init(st);

#endif
}

void
file_attr::init(fhandle_t hdl)
{
#if defined(_WIN32)

	BY_HANDLE_FILE_INFORMATION fi;

	if (!GetFileInformationByHandle(hdl, &fi)) {
		throw std::system_error(
			snf::system_error(),
			std::system_category(),
			"failed to get information from file handle");
	}

	init(fi, GetFileType(hdl));

#else // !_WIN32

	struct stat st;
	if (fstat(hdl, &st) < 0) {
		throw std::system_error(
			snf::system_error(),
			std::system_category(),
			"failed to stat file handle");
	}

	init(st);

#endif
}

#if defined(_WIN32)

void
file_attr::init(const WIN32_FIND_DATAW &fd)
{
	char *name = wcs2mbs(fd.cFileName);
	if (name && *name) {
		f_name = name;
		delete [] name;
	} else {
		throw std::runtime_error("invalid file name");
	}

	if (is_set(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
		f_type = file_type::directory;
	} else if (is_set(fd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
		if (is_set(fd.dwReserved0, IO_REPARSE_TAG_MOUNT_POINT))
			f_type = file_type::mntpoint;
		else if (is_set(fd.dwReserved0, IO_REPARSE_TAG_SYMLINK))
			f_type = file_type::symlink;
	} else if (is_set(fd.dwFileAttributes, FILE_ATTRIBUTE_NORMAL)) {
		f_type = file_type::regular;
	} else {
		std::ostringstream oss;
		oss << "invalid file attributes " << std::hex << fd.dwFileAttributes;
		throw std::runtime_error(oss.str());
	}

	f_mode = fd.dwFileAttributes;
	f_size = static_cast<int64_t>(combine(fd.nFileSizeHigh, fd.nFileSizeLow));
	f_ctime = file_time_to_epoch(fd.ftCreationTime); 
	f_atime = file_time_to_epoch(fd.ftLastAccessTime);
	f_mtime = file_time_to_epoch(fd.ftLastWriteTime);

	f_blksize = 8192;
	f_blkcnt = f_size / f_blksize;
	if ((f_size % f_blksize) != 0)
		f_blkcnt++;

	f_nlinks = 0;
	f_inode = 0;
	f_dev = 0;
	f_rdev = 0;
	f_uid = 0;
	f_gid = 0;
}

void
file_attr::init(const WIN32_FILE_ATTRIBUTE_DATA &fad)
{
	if (is_set(fad.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
		f_type = file_type::directory;
	} else if (is_set(fad.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
		f_type = file_type::symlink;
	} else if (is_set(fad.dwFileAttributes, FILE_ATTRIBUTE_NORMAL)) {
		f_type = file_type::regular;
	} else {
		std::ostringstream oss;
		oss << "invalid file attributes " << std::hex << fad.dwFileAttributes;
		throw std::runtime_error(oss.str());
	}

	f_mode = fad.dwFileAttributes;
	f_size = static_cast<int64_t>(combine(fad.nFileSizeHigh, fad.nFileSizeLow));
	f_ctime = file_time_to_epoch(fad.ftCreationTime); 
	f_atime = file_time_to_epoch(fad.ftLastAccessTime);
	f_mtime = file_time_to_epoch(fad.ftLastWriteTime);

	f_blksize = 8192;
	f_blkcnt = f_size / f_blksize;
	if ((f_size % f_blksize) != 0)
		f_blkcnt++;

	f_nlinks = 0;
	f_inode = 0;
	f_dev = 0;
	f_rdev = 0;
	f_uid = 0;
	f_gid = 0;
}

void
file_attr::init(const BY_HANDLE_FILE_INFORMATION &fi, DWORD ft)
{
	if (ft == FILE_TYPE_DISK) {
		if (is_set(fi.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
			f_type = file_type::directory;
		} else if (is_set(fi.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT)) {
			f_type = file_type::symlink;
		} else if (is_set(fi.dwFileAttributes, FILE_ATTRIBUTE_NORMAL)) {
			f_type = file_type::regular;
		} else {
			std::ostringstream oss;
			oss << "invalid file attributes " << std::hex << fi.dwFileAttributes;
			throw std::runtime_error(oss.str());
		}
	} else if (ft == FILE_TYPE_CHAR) {
		f_type = file_type::character;
	} else if (ft == FILE_TYPE_PIPE) {
		f_type = file_type::fifo;
	} else {
		std::ostringstream oss;
		oss << "invalid file type " << std::hex << ft;
		throw std::runtime_error(oss.str());
	}

	f_mode = fi.dwFileAttributes;
	f_size = static_cast<int64_t>(combine(fi.nFileSizeHigh, fi.nFileSizeLow));
	f_ctime = file_time_to_epoch(fi.ftCreationTime); 
	f_atime = file_time_to_epoch(fi.ftLastAccessTime);
	f_mtime = file_time_to_epoch(fi.ftLastWriteTime);

	f_blksize = 8192;
	f_blkcnt = f_size / f_blksize;
	if ((f_size % f_blksize) != 0)
		f_blkcnt++;

	f_nlinks = fi.nNumberOfLinks;
	f_inode = combine(fi.nFileIndexHigh, fi.nFileIndexLow);
	f_dev = fi.dwVolumeSerialNumber;
	f_rdev = 0;
	f_uid = 0;
	f_gid = 0;
}

#else // !_WIN32

void
file_attr::init(const struct stat &st)
{
	if (S_ISREG(st.st_mode))
		f_type = file_type::regular;
	else if (S_ISDIR(st.st_mode))
		f_type = file_type::directory;
	else if (S_ISCHR(st.st_mode))
		f_type = file_type::character;
	else if (S_ISBLK(st.st_mode))
		f_type = file_type::block;
	else if (S_ISFIFO(st.st_mode))
		f_type = file_type::fifo;
	else if (S_ISLNK(st.st_mode))
		f_type = file_type::symlink;
	else if (S_ISSOCK(st.st_mode))
		f_type = file_type::socket;
	else
		f_type = file_type::unknown;

	f_mode = st.st_mode;
	f_ctime = st.st_ctime;
	f_atime = st.st_atime;
	f_mtime = st.st_mtime;
	f_size = st.st_size;
	f_blksize = st.st_blksize;
	f_blkcnt = st.st_blocks;
	f_inode = st.st_ino;
	f_nlinks = st.st_nlink;
	f_dev = st.st_dev;
	f_rdev = st.st_rdev;
	f_uid = st.st_uid;
	f_gid = st.st_gid;
}

#endif

} // namespace snf
