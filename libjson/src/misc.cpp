#include "misc.h"
#include "utf.h"
#include "json.h"
#include <sstream>

namespace snf {
namespace json {

/*
 * Escapes a string. Escape characters including escaped multibyte
 * characters in \uxxxx format are replaced with the raw utf characters.
 * @throw std::invalid_argument
 */
std::string
string_unescape(const std::string &s)
{
	std::istringstream iss(s);
	iss.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	std::string str;
	char c;

	try {
		while (true) {
			iss.get(c);
			if (c == '\\') {
				iss.get(c);
				switch (c) {
					case '"':  str += '\"'; break;
					case '\\': str += '\\'; break;
					case '/':  str += '/';  break;
					case 'b':  str += '\b'; break;
					case 'f':  str += '\f'; break;
					case 'n':  str += '\n'; break;
					case 'r':  str += '\r'; break;
					case 't':  str += '\t'; break;
					case 'u':
						iss.putback('u');
						iss.putback('\\');
						str += utf16_decode(iss);
						break;

					default:
						throw std::invalid_argument("invalid escape sequence");
						break;
				}
			} else {
				str += c;
			}
		}
	} catch (const std::ios::failure &) {
		if (!iss.eof())
			throw;	
	}

	return str;
}

/*
 * Un-escapes a string. Special characters including raw multibyte utf
 * characters are escaped.
 * @throw std::invalid_argument
 */
std::string
string_escape(const std::string &s)
{
	std::istringstream iss(s);
	iss.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	std::string str;
	char c;

	try {
		while (true) {
			iss.get(c);
			if (c == '\\') {
				str += "\\\\";
			} else if (c == '\"') {
				str += "\\\"";
			} else if (c == '/') {
				str += "\\/";
			} else if (c == '\b') {
				str += "\\b";
			} else if (c == '\f') {
				str += "\\f";
			} else if (c == '\n') {
				str += "\\n";
			} else if (c == '\r') {
				str += "\\r";
			} else if (c == '\t') {
				str += "\\t";
			} else if (static_cast<unsigned char>(c) < 0x80) {
				str += c;
			} else if (static_cast<unsigned char>(c) >= 0x80) {
				iss.putback(c);
				str += utf16_encode(iss);
			} else {
				throw std::invalid_argument("invalid code point");
			}
		}
	} catch (const std::ios::failure &) {
		if (!iss.eof())
			throw;	
	}

	return str;
}

} // json
} // snf
