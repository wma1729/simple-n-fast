#include "common.h"

namespace snf {

constexpr char s_hex_table[] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
};

/*
 * Get the system error string.
 *
 * @param [out] str - the buffer to get the error string.
 * @param [in]  len - maximum buffer size.
 * @param [in]  err - the system error code.
 *
 * @return pointer to the error string or NULL in case of
 * failure.
 */
const char *
syserr(char *str, size_t len, int err)
{
	if (str == 0) {
		return 0;
	}

	str[0] = '\0';

	if (len == 0) {
		return 0;
	}

#if defined(_WIN32)

	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS |
		FORMAT_MESSAGE_MAX_WIDTH_MASK,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		str,
		static_cast<DWORD>(len),
		NULL);
	if (str[0] == '\0') {
		char *s = strerror(err);
		if(s != NULL) {
			strncpy(str, s, len);
			str[len] = '\0';
		}
	}

	len = strlen(str);
	while (len) {
		if (isspace(str[len - 1]) || isnewline(str[len - 1])) {
			str[--len] = '\0';
		} else {
			break;
		}
	}

#elif defined(__linux__)

	char tmpBuf[ERRSTRLEN + 1];
	char *s = strerror_r(err, tmpBuf, ERRSTRLEN);
	if (s) {
		strncpy(str, s, len);
	} else {
		snprintf(str, len, "Unknown error %d", err);
	}
	str[len] = '\0';

#endif

	return str;
}

/**
 * Get the base name of the file.
 *
 * @param [out] buf   - Buffer to get the file name.
 * @param [in] buflen - Buffer size
 * @param [in] path   - Path to extract the file name from.
 * @param [in] stripExt - Strip file name extension.
 *
 * @return pointer to the file name on success, NULL on
 * error.
 */
const char *
basename(char *buf, size_t buflen, const char *path, bool stripExt)
{
	if ((path == 0) || (*path == 0)) {
		return 0;
	}

	if ((buf == 0) || (buflen <= 0)) {
		return 0;
	}

	const char *ptr = strrchr(path, pathsep());
	if (ptr != 0) {
		ptr++;
	} else {
		ptr = path;
	}

	size_t idx = 0;
	while (ptr && *ptr && (idx < buflen)) {
		if (stripExt && (*ptr == '.'))
			break;
		buf[idx++] = *ptr++;
	}
	buf[idx] = '\0';

	return buf;
}

std::string
trim(const std::string &str)
{
	size_t b = str.find_first_not_of(" \f\n\r\t\v");
	if (b == std::string::npos)
		return std::string();

	size_t e = str.find_last_not_of(" \f\n\r\t\v");
	if (e == std::string::npos)
		e = str.size();

	return str.substr(b, e - b + 1);
}

std::string
bin2hex(const uint8_t *buf, size_t len)
{
	if ((buf == nullptr) || (len == 0))
		return std::string();

	std::string s(len * 2, ' ');

	for (size_t i = 0; i < len; ++i) {
		s[2 * i]     = s_hex_table[(buf[i] & 0xF0) >> 4];
		s[2 * i + 1] = s_hex_table[(buf[i] & 0x0F)];
	}

	return s;
}

} // namespace snf
