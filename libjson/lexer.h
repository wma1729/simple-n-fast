#ifndef _SNF_LEXER_H_
#define _SNF_LEXER_H_

#include "json.h"

namespace snf {
namespace json {

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

struct token
{
	kind	t_kind;
	value	t_value;

	token() : t_kind(kind::k_none), t_value() {}

};

class lexer
{
private:
	std::istream    &m_is;
	int             m_row;
	int             m_col;
	int             m_lastrowcol;
	token           m_token;

	char getc();
	void ungetc(char);
	void get_null();
	void get_true();
	void get_false();
	void get_number();
	void get_string();

public:
	lexer() = delete;
	lexer(std::istream &is)
		: m_is(is)
		, m_row(1)
		, m_col(0)
		, m_lastrowcol(0)
		, m_token() {}
	~lexer() {}
	token & get();
};

} // json
} // snf

#endif // _SNF_LEXER_H_
