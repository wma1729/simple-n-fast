#include "common.h"

/**
 * Get local system time.
 *
 * @param [out] lt Local time.
 *
 * @return UTC time since epoch.
 */
time_t
GetLocalTime(local_time_t *lt)
{
	time_t	now = 0;

#if defined(_WIN32)

	SYSTEMTIME      st;
	FILETIME        ft;
	ULARGE_INTEGER  current;
	ULARGE_INTEGER  epoch;

	GetLocalTime(&st);

	lt->year = st.wYear;
	lt->month = st.wMonth;
	lt->day = st.wDay;
	lt->hour = st.wHour;
	lt->minute = st.wMinute;
	lt->second = st.wSecond;
	lt->msec = st.wMilliseconds;

	epoch.QuadPart = 116444736000000000I64;

	SystemTimeToFileTime(&st, &ft);

	current.LowPart = ft.dwLowDateTime;
	current.HighPart = ft.dwHighDateTime;

	now = (time_t)((current.QuadPart - epoch.QuadPart) / 10000000);

#else /* !_WIN32 */

	struct timeval  tv;
	struct timezone tz;
	struct tm       *ptm;
	struct tm       tmStruct;

	gettimeofday(&tv, &tz);
	now = (time_t)(tv.tv_sec);
	ptm = localtime_r(&now, &tmStruct);

	lt->year = ptm->tm_year + 1900;
	lt->month = ptm->tm_mon + 1;
	lt->day = ptm->tm_mday;
	lt->hour = ptm->tm_hour;
	lt->minute = ptm->tm_min;
	lt->second = ptm->tm_sec;
	lt->msec = (tv.tv_usec == 0) ? 0 : (int)(tv.tv_usec / 1000);

#endif

	return now;
}

/**
 * Converts local time to string format.
 * - YYYY/MM/DD hh:mm:ss.mse
 *
 * @param [in]  lt     - local time.
 * @param [out] buf    - buffer to get the time string.
 * @param [in]  buflen - size of the buffer.
 *
 * @return string representation of the time on success,
 * NULL on failure.
 */
const char *
LocalTimeToString(local_time_t *lt, char *buf, size_t buflen)
{
	if ((lt == 0) || (buf == 0) || (buflen <= 24)) {
		return 0;
	}

	snprintf(buf, buflen, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
		lt->year, lt->month, lt->day,
		lt->hour, lt->minute, lt->second, lt->msec);

	return buf;
}

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
GetErrorStr(char *str, size_t len, int err)
{
	if (str == 0) {
		return 0;
	}

	str[0] = '\0';

	if (len == 0) {
		return 0;
	}

#if defined(_WIN32)

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, str, (DWORD)len, NULL);
	if (str[0] == '\0') {
		char *s = strerror(err);
		if(s != NULL) {
			strncpy(str, s, len);
			str[len] = '\0';
		}
	}

	len = strlen(str);
	while (len) {
		if (isspace(str[len - 1]) || ISNEWLINE(str[len - 1])) {
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
GetBaseName(char *buf, size_t buflen, const char *path, bool stripExt)
{
	if ((path == 0) || (*path == 0)) {
		return 0;
	}

	if ((buf == 0) || (buflen <= 0)) {
		return 0;
	}

	const char *ptr = strrchr(path, PATH_SEP);
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
