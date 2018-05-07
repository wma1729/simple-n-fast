#include "misc.h"
#include "utf.h"
#include <sstream>
#include <stdexcept>

namespace snf {
namespace json {

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
						throw_exception("invalid escape sequence");
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
			} else if (c <= 0x7F) {
				str += c;
			} else if ((c & 0xC0) == 0xC0) {
				iss.putback(c);
				str += utf16_encode(iss);
			} else {
				throw_exception("invalid code point");
			}
		}
	} catch (const std::ios::failure &) {
		if (!iss.eof())
			throw;	
	}

	return str;
}

void
throw_exception(const std::string &msg)
{
	std::ostringstream oss;
	oss << "parsing error; " << msg;
	throw std::runtime_error(oss.str());
}

void
throw_exception(const std::string &msg, int row, int col)
{
	std::ostringstream oss;
	oss << "parsing error at " << row << "." << col << "; " << msg;
	throw std::runtime_error(oss.str());
}

} // json
} // snf
