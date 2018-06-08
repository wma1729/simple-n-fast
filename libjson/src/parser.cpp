#include "json.h"
#include "lexer.h"
#include <fstream>
#include <sstream>

namespace snf {
namespace json {

static value parse_array(lexer &);
static value parse_object(lexer &);
static value parse(lexer &);

/*
 * Generates the JSON value from its string representation.
 * @throw std::invalid_argument
 * @throw std::ios_base::failure
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
value
from_string(const std::string &s)
{
	std::istringstream iss(s);
	iss.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	return from_stream(iss);
}

/*
 * Generates the JSON value from the file's content.
 * @throw std::invalid_argument
 * @throw std::ios_base::failure
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
value
from_file(const std::string &f)
{
	std::ifstream ifs;
	ifs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	ifs.open(f);
	return from_stream(ifs);
}

/*
 * Generates the JSON value from the input stream.
 * @throw std::invalid_argument
 * @throw std::ios_base::failure
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
value
from_stream(std::istream &is)
{
	lexer lex(is);
	return parse(lex);
}

/*
 * Parses JSON array.
 * @throw std::invalid_argument
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
static value
parse_array(lexer &lex)
{
	value val;
	array arr;
	token &t = lex.get();

	while (t.t_kind != kind::k_eof) {
		switch (t.t_kind) {
			case kind::k_null:
			case kind::k_true:
			case kind::k_false:
			case kind::k_integer:
			case kind::k_real:
			case kind::k_string:
				arr.add(t.t_value);
				break;

			case kind::k_lcb:
				arr.add(parse_object(lex));
				break;

			case kind::k_lb:
				arr.add(parse_array(lex));
				break;

			default:
				throw parsing_error("array element expected", lex.row(), lex.col());
				break;
		}

		t = lex.get();
		if (t.t_kind == kind::k_rb)
			break;
		else if (t.t_kind != kind::k_comma)
			throw parsing_error(", or ] expected", lex.row(), lex.col());
		else
			t = lex.get();
	}

	val = std::move(arr);
	return val;
}

/*
 * Parses JSON object.
 * @throw std::invalid_argument
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
static value
parse_object(lexer &lex)
{
	std::string key;
	value val;
	object obj;
	token &t = lex.get();

	while (t.t_kind != kind::k_eof) {
		if (t.t_kind != kind::k_string)
			throw parsing_error("string expected", lex.row(), lex.col());

		key = t.t_value.get_string();	

		t = lex.get();
		if (t.t_kind != kind::k_colon)
			throw parsing_error("colon expected", lex.row(), lex.col());

		t = lex.get();
		switch (t.t_kind) {
			case kind::k_null:
			case kind::k_true:
			case kind::k_false:
			case kind::k_integer:
			case kind::k_real:
			case kind::k_string:
				obj.add(key, t.t_value);
				break;

			case kind::k_lcb:
				obj.add(key, parse_object(lex));
				break;

			case kind::k_lb:
				obj.add(key, parse_array(lex));
				break;

			default:
				throw parsing_error("object member value expected",
					lex.row(), lex.col());
		}

		t = lex.get();
		if (t.t_kind == kind::k_rcb)
			break;
		else if (t.t_kind != kind::k_comma)
			throw parsing_error(", or } expected", lex.row(), lex.col());
		else
			t = lex.get();
	}

	val = std::move(obj);
	return val;
}

/*
 * Parses JSON value.
 * @throw std::invalid_argument
 * @throw snf::json::parsing_error
 * @return the JSON value.
 */
static value
parse(lexer &lex)
{
	const token &t = lex.get();
	if (t.t_kind == kind::k_lcb) {
		return parse_object(lex);
	} else if (t.t_kind == kind::k_lb) {
		return parse_array(lex);
	} else {
		throw parsing_error("only object and array are allowed as the top level JSON!",
			lex.row(), lex.col());
		return value();
	}
}

} // json
} // snf
