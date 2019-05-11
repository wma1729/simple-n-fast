#ifndef _SNF_CHARSET_H_
#define _SNF_CHARSET_H_

#include <cctype>

namespace snf {
namespace http {

inline bool
is_tchar(int c) noexcept
{
	bool valid = false;

	if (std::isalpha(c) || std::isdigit(c)) {
		valid = true;
	} else {
		switch (c) {
			case '!': case '#': case '$':
			case '%': case '&': case '\'':
			case '*': case '+': case '-':
			case '.': case '^': case '_':
			case '`': case '|': case '~':
				valid = true;
				break;

			default:
				break;
		}
	}

	return valid;
}

inline bool
is_vchar(int c) noexcept
{
	return ((c >= 0x21) && (c <= 0x7E));
}

inline bool
is_whitespace(int c) noexcept
{
	return ((c == ' ') || (c == '\t'));
}

inline bool
is_opaque(int c) noexcept
{
	return ((c >= 0x80) && (c <= 0xFF));
}

/*
 * 0x21 - 0x7E
 * Excluded:
 * - 0x22 "
 * - 0x5C \
 */
inline bool
is_quoted(int c) noexcept
{
	if (is_whitespace(c) || is_opaque(c))
		return true;
	else if ((c == 0x21) || ((c >= 0x23) && (c <= 0x5B)) || ((c >= 0x5D) && (c <= 0x7E)))
		return true;

	return false;
}

/*
 * 0x21 - 0x7E
 * Excluded:
 * - 0x28 (
 * - 0x29 )
 * - 0x5C \
 */
inline bool
is_commented(int c) noexcept
{
	if (is_whitespace(c) || is_opaque(c))
		return true;
	else if (((c >= 0x21) && (c <= 0x27)) || ((c >= 0x2A) && (c <= 0x5B)) || ((c >= 0x5D) && (c <= 0x7E)))
		return true;

	return false;
}

inline bool
is_escaped(int c) noexcept
{
	return is_whitespace(c) || is_vchar(c) || is_opaque(c);
}

} // namespace http
} // namespace snf

#endif // _SNF_CHARSET_H_
