#include "common.h"
#include "headers.h"
#include "status.h"
#include "charset.h"
#include "parseutil.h"
#include <sstream>
#include <algorithm>

namespace snf {
namespace http {

std::ostream &
operator<<(std::ostream &os, const headers &hdrs)
{
	for (auto &hdr : hdrs.m_headers)
		os << headers::canonicalize_name(hdr.first) << ": " << hdr.second->str() << "\r\n";
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
		if (name == I->first)
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
		if (name == I->first)
			return I;
	return m_headers.end();
}

/*
 * Canonicalize header field name.
 * content-length -> Content-Length
 * host -> Host
 */
std::string
headers::canonicalize_name(const std::string &name)
{
	std::string n;
	bool capitalize = true;

	for (size_t i = 0; i < name.length(); ++i) {
		int c = (capitalize) ? std::toupper(name[i]) : name[i];
		n.push_back(c);
		capitalize = (name[i] == '-');
	}

	return n;
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
header_field_value *
headers::validate(const std::string &name, const std::string &value)
{
	if (name == CONTENT_LENGTH) {
		return new number_value(value);
	} else if (name == TRANSFER_ENCODING) {
		return new token_list_value(value);
	} else if (name == TE) {
		return new token_list_value(value);
	} else if (name == TRAILERS) {
		return new string_list_value(value);
	} else if (name == HOST) {
		return new host_value(value);
	} else if (name == VIA) {
		return new via_list_value(value);
	} else if (name == CONNECTION) {
		std::unique_ptr<string_list_value> slist(new string_list_value(value));
		const std::vector<std::string> &values = slist->get();
		for (auto v : values) {
			if (!valid_connection(v)) {
				std::ostringstream oss;
				oss << "connection option " << v << " is not supported";
				throw not_implemented(oss.str());
			}
		}
		return slist.release();
	} else if (name == CONTENT_TYPE) {
		std::unique_ptr<media_type_value> mtv(new media_type_value(value));
		const media_type &mt = mtv->get();
		std::ostringstream oss;
		
		if (mt.m_type == CONTENT_TYPE_T_TEXT) {
			if (mt.m_subtype != CONTENT_TYPE_ST_PLAIN) {
				oss << "subtype " << mt.m_subtype
					<< " is not supported for type "
					<< CONTENT_TYPE_T_TEXT;
				throw not_implemented(oss.str());
			}
		} else if (mt.m_type == CONTENT_TYPE_T_APPLICATION) {
			if (mt.m_subtype != CONTENT_TYPE_ST_JSON) {
				oss << "subtype " << mt.m_subtype
					<< " is not supported for type "
					<< CONTENT_TYPE_T_APPLICATION;
				throw not_implemented(oss.str());
			}
		} else {
			oss << "type " << mt.m_type << " is not supported";
			throw not_implemented(oss.str());
		}

		return mtv.release();
	} else if (name == CONTENT_ENCODING) {
		std::unique_ptr<string_list_value> slist(new string_list_value(value));
		const std::vector<std::string> &values = slist->get();
		for (auto v : values) {
			if (!valid_encoding(v)) {
				std::ostringstream oss;
				oss << "content-encoding " << v << " is not supported";
				throw not_implemented(oss.str());
			}
		}
		return slist.release();
	} else if (name == CONTENT_LOCATION) {
		uri the_uri(value);
		// If the value is invalid, the above will throw an exception
		return new string_value(value);
	} else if (name == DATE) {
		return new date_value(value);
	} else {
		return new string_list_value(value);
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
	if (name == TRANSFER_ENCODING)
		return true;
	else if (name == TE)
		return true;
	else if (name == TRAILERS)
		return true;
	else if (name == VIA)
		return true;
	else if (name == CONTENT_ENCODING)
		return true;
	return false;
}

/*
 * Is valid connection value?
 */
bool
headers::valid_connection(const std::string &cnxn)
{
	return ((cnxn == CONNECTION_CLOSE) ||
		(cnxn == CONNECTION_KEEP_ALIVE) ||
		(cnxn == CONNECTION_UPGRADE));
}

/*
 * Is encoding valid?
 */
bool
headers::valid_encoding(const std::string &coding)
{
	return ((coding == CONTENT_ENCODING_COMPRESS) ||
		(coding == CONTENT_ENCODING_X_COMPRESS) ||
		(coding == CONTENT_ENCODING_GZIP) ||
		(coding == CONTENT_ENCODING_X_GZIP) ||
		(coding == CONTENT_ENCODING_DEFLATE));
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
	size_t i = 0;
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

	while (len && is_whitespace(istr[len - 1]))
		len--;

	name = std::move(parse_token(istr, i, len));
	if (name.empty())
		throw bad_message("no header field name");

	skip_spaces(istr, i, len);

	if (istr[i] != ':') {
		oss << "header field name (" << name << ") does not terminate with :";
		throw bad_message(oss.str());
	} else {
		i++;
	}

	if (i < len) {
		try {
			value = std::move(parse_generic(istr, i , len));
		} catch (const bad_message &ex) {
			std::ostringstream oss;
			oss << "bad header field value for (" << name << "): " << ex.what();
			throw bad_message(oss.str());
		}
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
	if (name.empty())
		return;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	std::unique_ptr<header_field_value> v(validate(n, value));
	if (v.get()) {
		hdr_vec_t::iterator I = find(n);
		if (I == m_headers.end()) {
			m_headers.push_back(std::make_pair(n, std::shared_ptr<header_field_value>(v.release())));
		} else if (allow_comma_separated_values(n)) {
			string_list_value *slist = 0;
			token_list_value *tlist = 0;
			via_list_value *vlist = 0;

			if ((slist = dynamic_cast<string_list_value *>(I->second.get())) != 0) {
				string_list_value *sval = dynamic_cast<string_list_value *>(v.get());
				if (sval)
					*slist += *sval;
			} else if ((tlist = dynamic_cast<token_list_value *>(I->second.get())) != 0) {
				token_list_value *tval = dynamic_cast<token_list_value *>(v.get());
				if (tval)
					*tlist += *tval;
			} else if ((vlist = dynamic_cast<via_list_value *>(I->second.get())) != 0) {
				via_list_value *vval = dynamic_cast<via_list_value *>(v.get());
				if (vval)
					*vlist += *vval;
			} else {
				std::ostringstream oss;
				oss << "header field (" << n << ") occurs multiple times";
				throw bad_message(oss.str());
			}
		} else {
			std::ostringstream oss;
			oss << "header field (" << n << ") occurs multiple times";
			throw bad_message(oss.str());
		}
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
	if (name.empty())
		return;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	header_field_value *v = validate(n, value);
	if (v) {
		hdr_vec_t::iterator I = find(n);
		if (I == m_headers.end()) {
			m_headers.push_back(std::make_pair(n, std::shared_ptr<header_field_value>(v)));
		} else {
			I->second.reset(v);
		}
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
headers::update(const std::string &name, header_field_value *value)
{
	if (name.empty())
		return;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	hdr_vec_t::iterator I = find(n);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, std::shared_ptr<header_field_value>(value)));
	} else {
		I->second.reset(value);
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
	if (name.empty())
		return;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	hdr_vec_t::iterator I = find(n);
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
	if (name.empty())
		return false;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	return (m_headers.end() != find(n));
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
const header_field_value *
headers::get(const std::string &name) const
{
	if (name.empty())
		throw std::out_of_range("header field name not specified");

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	hdr_vec_t::const_iterator it = find(n);
	if (it != m_headers.end()) {
		return it->second.get();
	} else {
		std::ostringstream oss;
		oss << "header field name (" << name << ") not found";
		throw std::out_of_range(oss.str());
	}
}

size_t
headers::content_length() const
{
	const number_value *numval = dynamic_cast<const number_value *>(get(CONTENT_LENGTH));
	return numval->get();
}

void
headers::content_length(size_t length)
{
	update(CONTENT_LENGTH, new number_value(length));
}

const std::vector<token> &
headers::transfer_encoding() const
{
	const token_list_value *tlist = dynamic_cast<const token_list_value *>(get(TRANSFER_ENCODING));
	return tlist->get();
}

void
headers::transfer_encoding(const token &token)
{
	update(TRANSFER_ENCODING, new token_list_value(token));
}

void
headers::transfer_encoding(const std::vector<token> &tokens)
{
	update(TRANSFER_ENCODING, new token_list_value(tokens));
}

void
headers::transfer_encoding(const std::string &coding)
{
	update(TRANSFER_ENCODING, new token_list_value(coding));
}

bool
headers::is_message_chunked() const
{
	hdr_vec_t::const_iterator it = find(TRANSFER_ENCODING);
	if (it != m_headers.end()) {
		const token_list_value *tlist = dynamic_cast<const token_list_value *>(it->second.get());
		if (tlist) {
			const std::vector<token> &tokens = tlist->get();
			for (auto t : tokens) {
				if (t.m_name == TRANSFER_ENCODING_CHUNKED)
					return true;
			}
		}
	}
	return false;
}

const std::vector<token> &
headers::te() const
{
	const token_list_value *tlist = dynamic_cast<const token_list_value *>(get(TE));
	return tlist->get();
}

void
headers::te(const token &token)
{
	update(TE, new token_list_value(token));
}

void
headers::te(const std::vector<token> &tokens)
{
	update(TE, new token_list_value(tokens));
}

void
headers::te(const std::string &coding)
{
	update(TE, new token_list_value(coding));
}

bool
headers::has_trailers() const
{
	hdr_vec_t::const_iterator it = find(TE);
	if (it != m_headers.end()) {
		const token_list_value *tlist = dynamic_cast<const token_list_value *>(it->second.get());
		if (tlist) {
			const std::vector<token> &tokens = tlist->get();
			for (auto t : tokens) {
				if (t.m_name == TRAILERS)
					return true;
			}
		}
	}
	return false;
}

const std::vector<std::string> &
headers::trailers() const
{
	const string_list_value *slist = dynamic_cast<const string_list_value *>(get(TRAILERS));
	return slist->get();
}

void
headers::trailers(const std::string &fields)
{
	update(TRAILERS, fields);
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
const std::string &
headers::host(in_port_t *port) const
{
	const host_value *h = dynamic_cast<const host_value *>(get(HOST));
	if (port)
		*port = h->port();
	return h->host();
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
	update(HOST, new host_value(uristr, port));
}

const std::vector<via> &
headers::intermediary() const
{
	const via_list_value *vlist = dynamic_cast<const via_list_value *>(get(VIA));
	return vlist->get();
}

void
headers::intermediary(const via &v)
{
	update(VIA, new via_list_value(v));
}

void
headers::intermediary(const std::vector<via> &vvec)
{
	update(VIA, new via_list_value(vvec));
}

void
headers::intermediary(const std::string &viastr)
{
	update(VIA, new via_list_value(viastr));
}

const std::vector<std::string> &
headers::connection() const
{
	const string_list_value *slist = dynamic_cast<const string_list_value *>(get(CONNECTION));
	return slist->get();
}

void
headers::connection(const std::string &cnxn)
{
	update(CONNECTION, cnxn);
}

const media_type &
headers::content_type() const
{
	const media_type_value *mt = dynamic_cast<const media_type_value *>(get(CONTENT_TYPE));
	return mt->get();
}

void
headers::content_type(const media_type &mt)
{
	update(CONTENT_TYPE, new media_type_value(mt));
}

void
headers::content_type(const std::string &type, const std::string &subtype)
{
	update(CONTENT_TYPE, new media_type_value(type, subtype));
}

const std::vector<std::string> &
headers::content_encoding() const
{
	const string_list_value *slist = dynamic_cast<const string_list_value *>(get(CONTENT_ENCODING));
	return slist->get();
}

void
headers::content_encoding(const std::string &coding)
{
	update(CONTENT_ENCODING, coding);
}

const std::vector<std::string> &
headers::content_language() const
{
	const string_list_value *slist = dynamic_cast<const string_list_value *>(get(CONTENT_LANGUAGE));
	return slist->get();
}

void
headers::content_language(const std::string &language)
{
	update(CONTENT_LANGUAGE, language);
}

uri
headers::content_location() const
{
	const string_value *sval = dynamic_cast<const string_value *>(get(CONTENT_LOCATION));
	return uri(sval->get());
}

void
headers::content_location(const uri &u)
{
	std::ostringstream oss;
	oss << u;
	update(CONTENT_LOCATION, oss.str());
}

void
headers::content_location(const std::string &uristr)
{
	update(CONTENT_LOCATION, uristr);
}

const snf::datetime &
headers::date() const
{
	const date_value *dval = dynamic_cast<const date_value *>(get(DATE));
	return dval->get();
}

void
headers::date(time_t t)
{
	update(DATE, new date_value(t));
}

void
headers::date(const snf::datetime &dt)
{
	update(DATE, new date_value(dt));
}

void
headers::date(const std::string &dtstr)
{
	update(DATE, new date_value(dtstr));
}

} // namespace http
} // namespace snf
