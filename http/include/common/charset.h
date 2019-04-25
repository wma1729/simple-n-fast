#ifndef _SNF_CHARSET_H_
#define _SNF_CHARSET_H_

#include <cctype>

namespace snf {
namespace http {

inline bool
is_tchar(int c)
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

} // namespace http
} // namespace snf

#endif // _SNF_CHARSET_H_
