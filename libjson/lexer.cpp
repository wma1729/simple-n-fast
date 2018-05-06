#include "lexer.h"
#include "misc.h"
#include "utf.h"
#include <cstring>
#include <cctype>
#include <sstream>

namespace snf {
namespace json {

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

void
lexer::get_null()
{
	char buf[5];

	m_is.get(buf, 5);
	if (strncmp(buf, "null", 4) != 0) {
		std::ostringstream oss;
		oss << "expected null, found " << buf;
		throw_exception(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_null;
	}
}

void
lexer::get_true()
{
	char buf[5];

	m_is.get(buf, 5);
	if (strncmp(buf, "true", 4) != 0) {
		std::ostringstream oss;
		oss << "expected true, found " << buf;
		throw_exception(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_true;
		m_token.t_value = true;
	}
}

void
lexer::get_false()
{
	char buf[6];

	m_is.get(buf, 6);
	if (strncmp(buf, "false", 5) != 0) {
		std::ostringstream oss;
		oss << "expected false, found " << buf;
		throw_exception(oss.str(), m_row, m_col);
	} else {
		m_token.t_kind = kind::k_false;
		m_token.t_value = false;
	}
}

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
			throw_exception("expected decimal point not found", m_row, m_col);
		} else {
			ungetc(c);
		}
	} else if (digit1_9(c)) {
		// it could also be a digit 1 - 9
		nstr += c;
	} else {
		throw_exception("unexpected character found", m_row, m_col);
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
			throw_exception("no digit after decimal point found", m_row, m_col);
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
			throw_exception("unexpected character found", m_row, m_col);
		}

		// get the digits
		while (digit(c = getc())) {
			no_exp_value = false;
			nstr += c;
		}

		ungetc(c);

		if (no_exp_value)
			throw_exception("no exponent value found", m_row, m_col);
	}

	if (dot || exp) {
		m_token.t_kind = kind::k_real;
		m_token.t_value = static_cast<double>(std::stold(nstr));
	} else {
		m_token.t_kind = kind::k_integer;
		m_token.t_value = static_cast<int64_t>(std::stoll(nstr));
	}
}

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
					throw_exception("invalid escape sequence", m_row, m_col);
					break;
			}
		} else if (c == '"') {
			m_token.t_kind = kind::k_string;
			m_token.t_value = std::move(str);
		} else {
			str += c;
		}
	}

	return;
}

token &
lexer::get()
{
	char c;

	m_token.t_kind = kind::k_none;
	m_token.t_value.reset();

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
				m_is.unget();
				get_number();
			} else if (c == '"') {
				get_string();
			} else {
				throw_exception("unexpected character found",
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
