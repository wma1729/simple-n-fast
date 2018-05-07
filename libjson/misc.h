#ifndef _SNF_MISC_H_
#define _SNF_MISC_H_

#include <string>

namespace snf {
namespace json {

/* Is space */
constexpr bool space(char c)
{
	return ((c == ' ') ||
		(c == '\f') ||
		(c == '\n') ||
		(c == '\r') ||
		(c == '\t') ||
		(c == '\v'));
}

/* Is digit: 1 - 9 */
constexpr bool digit1_9(char c)
{
	return ((c == '1') ||
		(c == '2') ||
		(c == '3') ||
		(c == '4') ||
		(c == '5') ||
		(c == '6') ||
		(c == '7') ||
		(c == '8') ||
		(c == '9'));
}

/* Is digit: 0 - 9 */
constexpr bool digit(char c)
{
	return ((c == '0') || digit1_9(c));
}

/* Is lower case hex digit: a - f */
constexpr bool lxdigit(char c)
{
	return ((c >= 'a') && (c <= 'f'));
}

/* Is upper case hex digit: A - F */
constexpr bool uxdigit(char c)
{
	return ((c >= 'A') && (c <= 'F'));
}

std::string string_unescape(const std::string &);
std::string string_escape(const std::string &);

} // json
} // snf

#endif // _SNF_MISC_H_
