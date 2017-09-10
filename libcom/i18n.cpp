#include <cstdlib>

#if defined(WINDOWS)

#include <Windows.h>

/**
 * Converts multi-byte string to wide-char string. The memory for the
 * wide-char string is dynamically allocated using malloc(3). It must be
 * freed by the caller using free(3). The conversion is done using
 * MultiByteToWideChar().
 *
 * @param [in] s - the multi-byte string.
 * @return the wide-char string on success, and NULL on failure.
 */
wchar_t *
MbsToWcs(const char *s)
{
	if ((s == 0) || (*s == '\0')) {
		return 0;
	}

	int nReq = MultiByteToWideChar(CP_ACP, 0, s, -1, 0, 0);
	if (nReq <= 0) {
		return 0;
	}

	wchar_t *ws = (wchar_t *)malloc(nReq * sizeof(wchar_t));
	if (ws) {
		int n = MultiByteToWideChar(CP_ACP, 0, s, -1, ws, nReq);
		if (n != nReq) {
			free(ws);
			ws = 0;
		}
	}

	return ws;
}

/**
 * Converts wide-char string to multi-byte string. The memory for the
 * multi-byte string is dynamically allocated using malloc(3). It must be
 * freed by the caller using free(3). The conversion is done using
 * WideCharToMultiByte().
 *
 * @param [in] ws - the wide-char string.
 * @return the multi-byte string on success, and NULL on failure.
 */
char *
WcsToMbs(const wchar_t *ws)
{
	if ((ws == 0) || (*ws == L'\0')) {
		return 0;
	}

	int nReq = WideCharToMultiByte(CP_ACP, 0, ws, -1, 0, 0, 0, 0);
	if (nReq <= 0) {
		return 0;
	}

	char *s = (char *)malloc(nReq);
	if (s) {
		int n = WideCharToMultiByte(CP_ACP, 0, ws, -1, s, nReq, 0, 0);
		if (n != nReq) {
			free(s);
			s = 0;
		}
	}

	return s;
}

#else

/**
 * Converts multi-byte string to wide-char string. The memory for the
 * wide-char string is dynamically allocated using malloc(3). It must be
 * freed by the caller using free(3). The conversion is done using mbstowcs().
 *
 * @param [in] s - the multi-byte string.
 * @return the wide-char string on success, and NULL on failure.
 */
wchar_t *
MbsToWcs(const char *s)
{
	if ((s == 0) || (*s == '\0')) {
		return 0;
	}

	size_t nReq = mbstowcs(0, s, 0);
	if (nReq <= 0) {
		return 0;
	}

	wchar_t *ws = (wchar_t *)malloc((nReq + 1) * sizeof(wchar_t));
	if (ws) {
		size_t n = mbstowcs(ws, s, nReq);
		if (n <= 0) {
			free(ws);
			ws = 0;
		} else {
			ws[n] = L'\0';
		}
	}

	return ws;
}

/**
 * Converts wide-char string to multi-byte string. The memory for the
 * multi-byte string is dynamically allocated using malloc(3). It must be
 * freed by the caller using free(3). The conversion is done using wcstombs().
 *
 * @param [in] ws - the wide-char string.
 * @return the multi-byte string on success, and NULL on failure.
 */
char *
WcsToMbs(const wchar_t *ws)
{
	if ((ws == 0) || (*ws == L'\0')) {
		return 0;
	}

	size_t nReq = wcstombs(0, ws, 0);
	if (nReq <= 0) {
		return 0;
	}

	char *s = (char *)malloc(nReq + 1);
	if (s) {
		size_t n = wcstombs(s, ws, nReq);
		if (n <= 0) {
			free(s);
			s = 0;
		} else {
			s[n] = '\0';
		}
	}

	return s;
}

#endif
