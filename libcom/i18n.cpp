#if defined(_WIN32)
#include <Windows.h>
#else
#include <cstdlib>
#endif

namespace snf {

#if defined(_WIN32)

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
mbs2wcs(const char *s)
{
	if ((s == 0) || (*s == '\0')) {
		return 0;
	}

	int nReq = MultiByteToWideChar(CP_ACP, 0, s, -1, 0, 0);
	if (nReq <= 0) {
		return 0;
	}

	wchar_t *ws = new wchar_t[nReq];
	int n = MultiByteToWideChar(CP_ACP, 0, s, -1, ws, nReq);
	if (n != nReq) {
		delete [] ws;
		ws = 0;
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
wcs2mbs(const wchar_t *ws)
{
	if ((ws == 0) || (*ws == L'\0')) {
		return 0;
	}

	int nReq = WideCharToMultiByte(CP_ACP, 0, ws, -1, 0, 0, 0, 0);
	if (nReq <= 0) {
		return 0;
	}

	char *s = new char[nReq];
	int n = WideCharToMultiByte(CP_ACP, 0, ws, -1, s, nReq, 0, 0);
	if (n != nReq) {
		delete [] s;
		s = 0;
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
mbs2wcs(const char *s)
{
	if ((s == 0) || (*s == '\0')) {
		return 0;
	}

	size_t nReq = mbstowcs(0, s, 0);
	if (nReq <= 0) {
		return 0;
	}

	wchar_t *ws = new wchar_t[nReq + 1];
	size_t n = mbstowcs(ws, s, nReq);
	if (n <= 0) {
		delete [] ws;
		ws = 0;
	} else {
		ws[n] = L'\0';
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
wcs2mbs(const wchar_t *ws)
{
	if ((ws == 0) || (*ws == L'\0')) {
		return 0;
	}

	size_t nReq = wcstombs(0, ws, 0);
	if (nReq <= 0) {
		return 0;
	}

	char *s = new char[nReq + 1];
	size_t n = wcstombs(s, ws, nReq);
	if (n <= 0) {
		delete [] s;
		s = 0;
	} else {
		s[n] = '\0';
	}

	return s;
}

#endif

} // snf
