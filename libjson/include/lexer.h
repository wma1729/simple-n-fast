#ifndef _SNF_LEXER_H_
#define _SNF_LEXER_H_

#include "json.h"

namespace snf {
namespace json {

/* Kind of lexical token */
enum class kind {
	k_none,       // no token yet
	k_lcb,        // left curly brace '{'
	k_rcb,        // right curly brace '}'
	k_lb,         // left bracket '['
	k_rb,         // right bracker ']'
	k_colon,      // colon ':'
	k_comma,      // simple comma ','
	k_null,       // keyword 'null'
	k_true,       // keyword 'true'
	k_false,      // keyward 'false'
	k_integer,    // integer value
	k_real,       // real value
	k_string,     // string value
	k_eof         // end of json input
};

/* The lexical token itself */
struct token
{
	kind	t_kind;     // kind of token
	value	t_value;    // value of token, if any

	token() : t_kind(kind::k_none), t_value() {}
};

/* Lexical analyzer */
class lexer
{
private:
	std::istream    &m_is;          // input stream
	int             m_row;          // current row #
	int             m_col;          // current column #
	int             m_lastrowcol;   // column # in the last row
	token           m_token;        // last token read

	char getc();
	void ungetc(char);
	void get_null();
	void get_true();
	void get_false();
	void get_number();
	void get_string();

public:
	lexer() = delete;

	/*
	 * Initializes the lexical analyzer with the input stream.
	 * @param [in] is - reference to the input stream.
	 */
	lexer(std::istream &is)
		: m_is(is)
		, m_row(1)
		, m_col(0)
		, m_lastrowcol(0)
		, m_token() {}

	~lexer() {}

	token & get();

	/* Fetches the current row. */
	int row() const { return m_row; }

	/* Fetches the current column. */
	int col() const { return m_col; }
};

} // json
} // snf

#endif // _SNF_LEXER_H_
