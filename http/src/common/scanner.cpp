#include <iostream>
#include "scanner.h"
#include "charset.h"
#include "uri.h"

namespace snf {
namespace http {

bool
scanner::read_space()
{
	int  c;
	bool found = false;

	if (is_whitespace(c = get())) {
		found = true;

		while (is_whitespace(c = get()))
			;
	}

	unget();

	return found;
}

bool
scanner::read_opt_space()
{
	int c;

	while (is_whitespace(c = get()))
		;
	unget();

	return true;
}

bool
scanner::read_special(int s)
{
	int c = get();

	if (c != s) {
		unget(); 
		return false;
	}

	return true;
}

bool
scanner::read_crlf()
{
	int  c = get();
	bool found = false;

	if (c == -1) {
		found = true;
	} else if (c == '\n') {
		found = true;
	} else if (c == '\r') {
		found = true;
		c = get();
		if (c != '\n')
			unget();
	} else {
		unget();
	}

	return found;
}

bool
scanner::read_token(std::string &token, bool lower)
{
	int c;

	while (is_tchar(c = get()))
		token.push_back(lower ? std::tolower(c) : c);
	unget();

	return !token.empty();
}

bool
scanner::read_uri(std::string &u)
{
	int  c;
	bool next = true;

	while (next) {
		c = get();
		if (uri_reserved_character(c) || uri_unreserved_character(c) || (c == '%')) {
			u.push_back(c);
		} else {
			unget();
			next = false;
		}
	}

	if (u.empty())
		return false;

	uri the_uri{u};
	// If the URI is not formatted correctly, snf::http::bad_uri exception is thrown

	return true;
}

bool
scanner::read_version(std::string &version)
{
	int  c;
	bool next = true;

	while (next) {
		c = get();
		if (is_tchar(c) || (c == '/')) {
			version.push_back(c);
		} else {
			unget();
			next = false;
		}
	}

	return !version.empty();
}

bool
scanner::read_status(std::string &status)
{
	int c;

	while (isdigit(c = get()))
		status.push_back(c);
	unget();

	return (status.size() == 3);
}

bool
scanner::read_reason(std::string &reason)
{
	int         c;
	bool        next = true;
	std::string r;

	while (next) {
		c = get();
		if (is_whitespace(c) || is_vchar(c) || is_opaque(c)) {
			r.push_back(c);
		} else {
			unget();
			next = false;
		}
	}

	reason = std::move(snf::trim(r));

	return !reason.empty();
}

bool
scanner::read_qstring(std::string &qstr)
{
	int  c;
	bool next = true;
	bool valid = true;

	c = get();
	if (c != '"') {
		unget();
		return false;
	}

	qstr.push_back(c);

	while (next && valid) {
		c = get();
		if (c == '"') {
			qstr.push_back(c);
			next = false;
		} else if (c == '\\') {
			qstr.push_back(c);

			c = get();
			if (is_escaped(c))
				qstr.push_back(c);
			else
				valid = false;
		} else if (is_quoted(c)) {
			qstr.push_back(c);
		} else {
			valid = false;
		}
	}

	if (!valid) {
		std::ostringstream oss;
		oss << "invalid character \'" << c << "\' found in quoted-string " << qstr;
		throw std::invalid_argument(oss.str());
	}

	return true;
}

bool
scanner::read_comments(std::string &comments)
{
	int  c;
	bool next = true;
	bool valid = true;

	c = get();
	if (c != '(') {
		unget();
		return false;
	}

	comments.push_back(c);

	while (next && valid) {
		c = get();
		if (c == ')') {
			comments.push_back(c);
			next = false;
		} else if (c == '\\') {
			comments.push_back(c);

			c = get();
			if (is_escaped(c))
				comments.push_back(c);
			else
				valid = false;
		} else if (is_commented(c)) {
			comments.push_back(c);
		} else {
			valid = false;
		}
	}

	if (!valid) {
		std::ostringstream oss;
		oss << "invalid character \'" << c << "\' found in comments " << comments;
		throw std::invalid_argument(oss.str());
	}

	return true;
}

bool
scanner::read_parameters(param_vec_t &pvec)
{
	bool next = true;

	while (next) {
		std::string name, value;

		if (!read_token(name))
			throw std::invalid_argument("no parameter name");

		read_opt_space();

		if (!read_special('='))
			throw std::invalid_argument("no \'=\' after parameter name");

		read_opt_space();

		int c = peek();
		if (c == '"')
			next = read_qstring(value);
		else if (is_tchar(c))
			next = read_token(value, false);
		else
			next = false;

		if (!next) {
			throw std::invalid_argument("no/invalid parameter value");
		}

		pvec.push_back(std::make_pair(std::move(name), std::move(value)));

		read_opt_space();

		if (!read_special(';'))
			next = false;
	}

	return !pvec.empty();
}

bool
scanner::read_all(std::string &line)
{
	int         c;
	bool        next = true;
	std::string l;

	while (next) {
		c = get();
		if ((c == '\r') || (c == '\n')) {
			unget();
			next = false;
		} else if (c == -1) {
			next = false;
		} else {
			l.push_back(c);
		}
	}

	line = std::move(snf::trim(l));

	return true;
}

} // namespace http
} // namespace snf
