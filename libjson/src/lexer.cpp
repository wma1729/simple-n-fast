#include "lexer.h"
#include "misc.h"
#include "utf.h"
#include <cstring>
#include <cctype>
#include <sstream>

namespace snf {
namespace json {

/*
 * Gets a character from the input stream.
 * Row/column number are updated.
 */
char
lexer::getc()
{
	char c;

	m_is.get(c);
	if (c == '\n') {
		m_row++;
		m_lastrowcol = m_col;
		m_col = 0;
	} else {
		m_col++;
	}

	return c;
}

/*
 * Puts back a character into the input stream.
 * Row/column number are updated.
 */
void
lexer::ungetc(char c)
{
	m_is.putback(c);
	if (c == '\n') {
		m_row--;
		m_col = m_lastrowcol;
		m_lastrowcol = 0;
	} else {
		m_col--;
	}
}

/*
 * Reads the null token.
 * @throw snf::json::parsing_error
 */
void
lexer::get_null()
{
	char buf[5];

	m_is.get(buf, 5);
	if (strncmp(buf, "null", 4) != 0) {
		std::ostringstream oss;
		oss << "expected null, found " << buf;
		throw parsing_error(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_null;
	}
}

/*
 * Reads the true token.
 * @throw snf::json::parsing_error
 */
void
lexer::get_true()
{
	char buf[5];

	m_is.get(buf, 5);
	if (strncmp(buf, "true", 4) != 0) {
		std::ostringstream oss;
		oss << "expected true, found " << buf;
		throw parsing_error(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_true;
		m_token.t_value = true;
	}
}

/*
 * Reads the false token.
 * @throw snf::json::parsing_error
 */
void
lexer::get_false()
{
	char buf[6];

	m_is.get(buf, 6);
	if (strncmp(buf, "false", 5) != 0) {
		std::ostringstream oss;
		oss << "expected false, found " << buf;
		throw parsing_error(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_false;
		m_token.t_value = false;
	}
}

/*
 * Reads the number token. It could be integer or real.
 * @throw snf::json::parsing_error
 */
void
lexer::get_number()
{
	char c;
	bool dot = false;
	bool exp = false;
	std::string nstr;

	// check if negative number '-'
	c = getc();
	if (c == '-') {
		nstr += c;
		c = getc();
	}

	if (c == '0') {
		// if it starts with '0', the next character must be '.'
		nstr += c;
		if ((c = getc()) != '.') {
			throw parsing_error("expected decimal point not found", m_row, m_col);
		} else {
			ungetc(c);
		}
	} else if (digit1_9(c)) {
		// it could also be a digit 1 - 9
		nstr += c;
	} else {
		throw parsing_error("unexpected character found", m_row, m_col);
	}

	// get the digits
	while (digit(c = getc()))
		nstr += c;

	// decimal point
	if (c == '.') {
		dot = true;
		nstr += c;

		// make sure there is atleast one digit after decimal point;
		c = getc();
		if (digit(c)) {
			nstr += c;
		} else {
			throw parsing_error("no digit after decimal point found", m_row, m_col);
		}

		// get the digits following the decimal point
		while (digit(c = getc()))
			nstr += c;
	}

	if ((c == 'e') || (c == 'E')) {
		bool no_exp_value = true;
		exp = true;
		nstr += c;

		c = getc();
		if ((c == '+') || (c == '-')) {
			nstr += c;
			c = getc();
		}

		if (c == '0') {
			// skip it and subsequent zeroes
			while ((c = getc()) == '0')
				;
			ungetc(c);
		} else if (digit1_9(c)) {
			no_exp_value = false;
			nstr += c;
		} else {
			throw parsing_error("unexpected character found", m_row, m_col);
		}

		// get the digits
		while (digit(c = getc())) {
			no_exp_value = false;
			nstr += c;
		}

		if (no_exp_value)
			throw parsing_error("no exponent value found", m_row, m_col);
	}

	ungetc(c);

	if (dot || exp) {
		m_token.t_kind = kind::k_real;
		m_token.t_value = static_cast<double>(std::stold(nstr));
	} else {
		m_token.t_kind = kind::k_integer;
		m_token.t_value = static_cast<int64_t>(std::stoll(nstr));
	}
}

/*
 * Reads the string token. The string is un-escaped.
 * @throw std::invalid_argument
 * @throw snf::json::parsing_error
 */
void
lexer::get_string()
{
	char c;
	std::string str;

	while (true) {
		c = getc();
		if (c == '\\') {
			c = getc();
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
					ungetc('u');
					ungetc('\\');
					str += utf16_decode(m_is);
					break;
				default:
					throw parsing_error("invalid escape sequence",
						m_row, m_col);
					break;
			}
		} else if (c == '"') {
			m_token.t_kind = kind::k_string;
			m_token.t_value = std::move(str);
			break;
		} else {
			str += c;
		}
	}

	return;
}

/*
 * Gets the next token from the input stream.
 * @throw std::invalid_argument
 * @throw snf::json::parsing_error
 */
token &
lexer::get()
{
	char c;

	m_token.t_kind = kind::k_none;
	m_token.t_value = nullptr;

	try {
		while (true) {
			c = getc();

			if (space(c)) {
				continue;
			} else if (c == '{') {
				m_token.t_kind = kind::k_lcb;
			} else if (c == '}') {
				m_token.t_kind = kind::k_rcb;
			} else if (c == '[') {
				m_token.t_kind = kind::k_lb;
			} else if (c == ']') {
				m_token.t_kind = kind::k_rb;
			} else if (c == ':') {
				m_token.t_kind = kind::k_colon;
			} else if (c == ',') {
				m_token.t_kind = kind::k_comma;
			} else if (c == 'n') {
				ungetc(c);
				get_null();
			} else if (c == 't') {
				ungetc(c);
				get_true();
			} else if (c == 'f') {
				ungetc(c);
				get_false();
			} else if ((c == '-') || digit(c)) {
				ungetc(c);
				get_number();
			} else if (c == '"') {
				get_string();
			} else {
				throw parsing_error("unexpected character found",
					m_row, m_col);
			}
			break;
		}
	} catch (const std::ios::failure &) {
		if (m_is.eof())
			m_token.t_kind = kind::k_eof;
		else
			throw;	
	}


	return m_token;
}

} // json
} // snf
