#include "common.h"
#include "headers.h"
#include "status.h"
#include "charset.h"
#include "uri.h"
#include <iostream>
#include <sstream>

namespace snf {
namespace http {

std::ostream &
operator<<(std::ostream &os, const headers &hdrs)
{
	for (auto &hdr : hdrs.m_headers)
		os << hdr.first << ": " << hdr.second << "\r\n";
	return os;
}

/*
 * Find the field name in the headers.
 *
 * @param [in] name - header field name.
 *
 * @return iterator to the field name if found.
 */         
hdr_vec_t::iterator
headers::find(const std::string &name)
{
	for (hdr_vec_t::iterator I = m_headers.begin(); I != m_headers.end(); ++I)
		if (snf::streq(name, I->first, true))
			return I;
	return m_headers.end();
}

/*
 * Find the field name in the headers.
 *
 * @param [in] name - header field name.
 *
 * @return iterator to the field name if found.
 */         
hdr_vec_t::const_iterator
headers::find(const std::string &name) const
{
	for (hdr_vec_t::const_iterator I = m_headers.begin(); I != m_headers.end(); ++I)
		if (snf::streq(name, I->first, true))
			return I;
	return m_headers.end();
}

/*
 * Validate the header field name and/or value.
 *
 * @param [in] name  - header field name.
 * @param [in] value - header field value.
 *
 * @throws snf::http::not_implemented if the header field
 *         name and/or value is not implemented.
 * @throws snf::http::bad_message if the header field
 *         value is not set correctly.
 */
void
headers::validate(const std::string &name, const std::string &value)
{
	if (snf::streq(name, CONTENT_LENGTH)) {
		for (size_t i = 0; i < value.length(); ++i) {
			if (!isdigit(value[i])) {
				std::ostringstream oss;
				oss << "incorrect " << CONTENT_LENGTH << " (" << value << ") specified";
				throw bad_message(oss.str());
			}
		}
	} else if (snf::streq(name, TRANSFER_ENCODING)) {
		if (!snf::streq(value, TRANSFER_ENCODING_CHUNKED, true)) {
			throw not_implemented("only chunked transfer encoding is supported");
		}
	} else if (snf::streq(name, CONNECTION)) {
		if (!snf::streq(value, CONNECTION_CLOSE, true) &&
			!snf::streq(value, CONNECTION_KEEP_ALIVE, true) &&
			!snf::streq(value, CONNECTION_UPGRADE, true)) {

			std::ostringstream oss;
			oss << "connection option " << value << " is not supported";
			throw not_implemented(oss.str());
		}
	} else if (snf::streq(name, CONTENT_TYPE)) {
		// If the value is incorrect, an exception will be thrown
		media_type mt(value);
	} else if (snf::streq(name, CONTENT_ENCODING)) {
		if (!snf::streq(value, CONTENT_ENCODING_GZIP, true)) {
			throw not_implemented("only gzip content encoding is supported");
		}
	}
}

/*
 * Determines if the header field allows comma-separated values.
 *
 * @param [in] name - header field name.
 *
 * @return true if comma-separated values are allowed, false otherwise.
 */
bool
headers::allow_comma_separated_values(const std::string &name)
{
	if (snf::streq(name, TRANSFER_ENCODING))
		return true;
	else if (snf::streq(name, TE))
		return true;
	else if (snf::streq(name, TRAILERS))
		return true;
	return false;
}

/*
 * Parse the header line and add the name/value pair to the
 * headers list.
 *
 * @param [in] istr - HTTP header line.
 *
 * @throws snf::http::bad_message if the header line could
 *         not be parsed.
 *         snf::http::not_implemented if the header field
 *         name and/or value is not implemented.
 */
void
headers::add(const std::string &istr)
{
	size_t i;
	size_t len = istr.size();
	std::string name;
	std::string value;
	std::ostringstream oss;

	while (len > 2) {
		if ((istr[len - 1] == '\n') || (istr[len - 2] == '\r'))
			len -= 2;
		else
			break;
	}

	while (is_whitespace(istr[len - 1]))
		len--;

	for (i = 0; i < len; ++i) {
		if (!is_tchar(istr[i]))
			break;
		name.push_back(istr[i]);
	}

	if (name.empty())
		throw bad_message("no header field name");

	if (i >= len) {
		oss << "no header field value for field name (" << name << ")";
		throw bad_message(oss.str());
	}

	if (istr[i] != ':') {
		oss << "header field name (" << name << ") does not terminate with :";
		throw bad_message(oss.str());
	}

	for (i = i + 1; i < len; ++i)
		if (!is_whitespace(istr[i]))
			break;

	try {
		value = std::move(parse_generic(istr, i , len));
	} catch (const bad_message &ex) {
		std::ostringstream oss;
		oss << "bad header field value for (" << name << "): " << ex.what();
		throw bad_message(oss.str());
	}

	if (i != len) {
		oss << "invalid header field value for " << name;
		if (!value.empty())
			oss << " (" << value << ")";
		throw bad_message(oss.str());
	}

	add(name, snf::trim(value));
}

/*
 * Add the HTTP header name/value to the headers list.
 * If the header name already exists, the value passed is
 * appended to the header value.
 *
 * @param [in] name  - HTTP header name.
 * @param [in] value - HTTP header value.
 */
void
headers::add(const std::string &name, const std::string &value)
{
	validate(name, value);

	hdr_vec_t::iterator I = find(name);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, value));
	} else if (allow_comma_separated_values(name)) {
		I->second.append(", ");
		I->second.append(value);
	} else {
		std::ostringstream oss;
		oss << "header field (" << name << ") occurs multiple times";
		throw bad_message(oss.str());
	}
}

/*
 * Updates the HTTP header name/value in the headers list.
 * If the header name already exists, the value passed is
 * overrides the existing header value. If the header name
 * does not already exist, header name/value are added to
 * the headers list.
 *
 * @param [in] name  - HTTP header name.
 * @param [in] value - HTTP header value.
 */
void
headers::update(const std::string &name, const std::string &value)
{
	validate(name, value);

	hdr_vec_t::iterator I = find(name);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, value));
	} else {
		I->second = value;
	}
}

/*
 * Removes the HTTP header field name from the headers list.
 *
 * @param [in] name - HTTP header name.
 */
void
headers::remove(const std::string &name)
{
	hdr_vec_t::iterator I = find(name);
	if (I != m_headers.end())
		m_headers.erase(I);
}

/*
 * Determines if the HTTP header field name is present
 * in the headers list.
 *
 * @param [in] name - HTTP header name.
 *
 * @return true if the HTTP header field name is present,
 *         false otherwise.
 */
bool
headers::is_set(const std::string &name) const
{
	return (m_headers.end() != find(name));
}

/*
 * Get the HTTP header field value for the given
 * HTTP header field name.
 *
 * @param [in] name - HTTP header name.
 *
 * @return HTTP header value.
 *
 * @throws std::out_of_range if the name is not
 *         present in the headers list.
 */
std::string
headers::get(const std::string &name) const
{
	hdr_vec_t::const_iterator it = find(name);
	if (it != m_headers.end()) {
		return it->second;
	} else {
		std::ostringstream oss;
		oss << "header field name (" << name << ") not found";
		throw std::out_of_range(oss.str());
	}
}

size_t
headers::content_length() const
{
	std::string s = std::move(get(CONTENT_LENGTH));
	return std::stoull(s);
}

void
headers::content_length(size_t length)
{
	update(CONTENT_LENGTH, std::to_string(length));
}

std::string
headers::transfer_encoding() const
{
	return get(TRANSFER_ENCODING);
}

void
headers::transfer_encoding(const std::string &coding)
{
	update(TRANSFER_ENCODING, coding);
}

bool
headers::is_message_chunked() const
{
	hdr_vec_t::const_iterator it = find(TRANSFER_ENCODING);
	if (it != m_headers.end())
		if (snf::streq(it->second, TRANSFER_ENCODING_CHUNKED, true))
			return true;
	return false;
}

/*
 * Gets the value of the host header.
 *
 * @param [out] port - optional out port if not nullptr
 *                     and port is specified in the Host
 *                     field.
 *
 * @return the host name from the Host header field.
 *
 * @throws snf::http::bad_message if the host name cannot
 *         be parsed.
 */
std::string
headers::host(in_port_t *port) const
{
	hdr_vec_t::const_iterator it = find(HOST);
	if (it == m_headers.end())
		return std::string();

	std::string value(it->second);

	// make it a URI so that the URI parsing code can be used
	value.insert(0, "http://");

	try {
		snf::http::uri the_uri(value);

		if (port && the_uri.get_port().is_present())
			*port = the_uri.get_port().numeric_port();

		return the_uri.get_host().get();
	} catch (snf::http::bad_uri &ex) {
		throw snf::http::bad_message(ex.what());
	}
}

/*
 * Sets the value of host header.
 *
 * @param [in] uristr - Host name or URI.
 * @param [in] port   - Optional port to use. If not 0,
 *                      this is the port used. If 0,
 *                      the port value from URI, if present,
 *                      is used.
 *
 * @throws snf::http::bad_message if the host/URI name cannot
 *         be parsed. std::invalid_argument if the host component
 *         is missing in the URL.
 */
void
headers::host(const std::string &uristr, in_port_t port)
{
	try {
		std::string value;
		snf::http::uri the_uri(uristr);

		if (the_uri.get_host().is_present()) {
			value = the_uri.get_host().get();
		} else {
			std::ostringstream oss;
			oss << "invalid URI string specified: " << uristr;
			throw std::invalid_argument(oss.str());
		}

		if (port != 0) {
			value += ":";
			value += std::to_string(port);
		} else if (the_uri.get_port().is_present()) {
			value += the_uri.get_port().get();
			value += ":";
		}

		update(HOST, value);
	} catch (snf::http::bad_uri &ex) {
		throw snf::http::bad_message(ex.what());
	}
}

std::string
headers::connection() const
{
	return get(CONNECTION);
}

void
headers::connection(const std::string &cnxn)
{
	update(CONNECTION, cnxn);
}

media_type
headers::content_type() const
{
	std::string str = std::move(get(CONTENT_TYPE));
	return media_type(str);
}

void
headers::content_type(const media_type &mt)
{
	std::ostringstream oss;
	oss << mt;
	update(CONTENT_TYPE, oss.str());
}

std::string
headers::content_encoding() const
{
	return get(CONTENT_ENCODING);
}

void
headers::content_encoding(const std::string &coding)
{
	update(CONTENT_ENCODING, coding);
}

std::vector<std::string>
headers::content_language() const
{
	std::vector<std::string> languages;

	std::string lang = get(CONTENT_LANGUAGE);
	if (!lang.empty()) {
		std::istringstream iss(lang);
		std::string language;

		while (std::getline(iss, language, ','))
			languages.push_back(language);

	}

	return std::move(languages);
}

void
headers::content_language(const std::string &language)
{
	update(CONTENT_LANGUAGE, language);
}

void
headers::content_language(const std::vector<std::string> &languages)
{
	std::ostringstream oss;

	std::vector<std::string>::const_iterator it;
	for (it = languages.begin(); it != languages.end(); ++it) {
		if (it != languages.begin())
			oss << ", ";
		oss << *it;
	}

	update(CONTENT_LANGUAGE, oss.str());
}

} // namespace http
} // namespace snf
