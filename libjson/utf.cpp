#include "utf.h"
#include "misc.h"
#include <istream>
#include <sstream>
#include <iomanip>

namespace snf {
namespace json {

/*
 * Encodes a codepoint to UTF-8 encoding.
 *
 * UTF-8 encoding
 *
 * Byte  |  First  |   Last   | Byte 1   | Byte 2   | Byte 3   | Byte 4 
 * Count |   CP    |    CP    |          |          |          |
 * ------+---------+----------+----------+----------+----------+----------
 * 1     |  U+0000 |   U+007F | 0xxxxxxx |          |          |
 * 2     |  U+0080 |   U+07FF | 110xxxxx | 10xxxxxx |          |
 * 3     |  U+0800 |   U+FFFF | 1110xxxx | 10xxxxxx | 10xxxxxx |
 * 4     | U+10000 | U+10FFFF | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx
 */
std::string
utf8_encode(uint32_t codepoint)
{
	std::string str;

	if ((codepoint < 0) || (codepoint > 0x10FFFF)) {
		std::ostringstream oss;
		oss.setf(std::ios::showbase);
		oss << "invalid code point (" << std::hex << codepoint << ")";
		throw std::runtime_error(oss.str());
	} else if (codepoint < 0x80) {
		str += static_cast<char>(codepoint);
	} else if (codepoint < 0x800) {
		str += static_cast<char>(0xC0 + ((codepoint & 0x7C0) >> 6));
		str += static_cast<char>(0x80 + (codepoint & 0x03F));
	} else if (codepoint < 0x10000) {
		str += static_cast<char>(0xE0 + ((codepoint & 0xF000) >> 12));
		str += static_cast<char>(0x80 + ((codepoint & 0x0FC0) >> 6));
		str += static_cast<char>(0x80 + (codepoint & 0x003F));
	} else if (codepoint <= 0x10FFFF) {
		str += static_cast<char>(0xF0 + ((codepoint & 0x1C0000) >> 18));
		str += static_cast<char>(0x80 + ((codepoint & 0x03F000) >> 12));
		str += static_cast<char>(0x80 + ((codepoint & 0x000FC0) >> 6));
		str += static_cast<char>(0x80 + (codepoint & 0x00003F));
	}

	return str;
}

/*
 * Converts a 4-digit hex string to a 16-bit number.
 */
static int
utf16_decode_seq(std::istream &is)
{
	char c1, c2;
	int w = 0;

	is.get(c1);
	is.get(c2);

	if ((c1 != '\\') && (c2 != 'u'))
		return -1;

	for (int i = 0; i < 4; ++i) {
		w <<= 4;

		is.get(c1);
		if (digit(c1)) {
			w += c1 - '0';
		} else if (lxdigit(c1)) {
			w += c1 - 'a' + 10;
		} else if (uxdigit(c1)) {
			w += c1 - 'A' + 10;
		} else {
			w = -1;
			break;
		}
	}

	return w;
}

/*
 * Surrogates are characters in the Unicode range U+D800 - U+DFFF (2048 code points):
 * - U+D800 - U+DBFF (1,024 code points): high surrogates
 * - U+DC00 - U+DFFF (1,024 code points): low surrogates
 *
 * In UTF-16, characters in ranges U+0000 - U+D7FF and U+E000 - U+FFFD are stored as
 * a single 16 bits unit. Characters (range U+10000 - U+10FFFF) are stored as surrogate pairs,
 * two 16 bits units:
 * - an high surrogate (in range U+D800 - U+DBFF) followed by
 * - a low surrogate (in range U+DC00 - U+DFFF).
 */
std::string
utf16_decode(std::istream &is)
{
	uint32_t dw = 0;
	int w1 = 0;
	int w2 = 0;

	w1 = utf16_decode_seq(is);
	if (w1 == -1)
		throw std::runtime_error("invalid UTF-16 main sequence");

	dw = w1;

	if ((w1 >= 0xD800) && (w1 <= 0xDBFF)) {

		w2 = utf16_decode_seq(is);
		if (w2 == -1)
			throw std::runtime_error("invalid UTF-16 surrogate sequence");

		if ((w2 >= 0xDC00) && (w1 <= 0xDFFF)) {
			dw = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;
		} else {
			std::ostringstream oss;
			oss.setf(std::ios::showbase);
			oss << "invalid UTF-16 surrogate sequence (" << std::hex << w2 << ")";
			throw std::runtime_error(oss.str());
		}
	} else {
		std::ostringstream oss;
		oss.setf(std::ios::showbase);
		oss << "invalid UTF-16 main sequence (" << std::hex << w1 << ")";
		throw std::runtime_error(oss.str());
	}

	return utf8_encode(dw);
}

std::string
utf16_encode(std::istream &is)
{
	uint32_t dw = 0;
	int len = 0;
	char c;

	is.get(c);
	if ((c & 0xF0) == 0xF0) {
		len = 4;
		dw = c & 0x07;
	} else if ((c & 0xE0) == 0xE0) {
		len = 3;
		dw = c & 0x0F;
	} else if ((c & 0xC0) == 0xC0) {
		len = 2;
		dw = c & 0x1F;
	} else {
		len = 1;
		dw = c;
	}

	for (int i = 1; i < len; ++i) {
		is.get(c);
		if ((c & 0x80) == 0x80) {
			dw = (dw << 6) | (c & 0x3F);
		} else {
			std::ostringstream oss;
			oss.setf(std::ios::showbase);
			oss << "invalid UTF-8 sequence (" << std::hex << c << ")";
			throw std::runtime_error(oss.str());
		}
	}

	std::ostringstream oss;

	if (dw < 0x10000) {
		oss << "\\u" << std::setw(4) << std::setfill('0') << std::hex << dw;
	} else {
		int w1, w2;

		dw -= 0x10000;
		w1 = 0xD800 | ((dw & 0xFFC00) >> 10);
		w2 = 0xDC00 | (dw & 0x003FF);
		oss << "\\u" << std::setw(4) << std::setfill('0') << std::hex << w1;
		oss << "\\u" << std::setw(4) << std::setfill('0') << std::hex << w2;
	}

	return oss.str();
}

} // json
} // snf
