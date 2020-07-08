#include "common.h"
#include "headers.h"
#include "charset.h"
#include "status.h"
#include "scanner.h"
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
	for (hdr_vec_t::const_iterator I = m_headers.cbegin(); I != m_headers.cend(); ++I)
		if (name == I->first)
			return I;
	return m_headers.cend();
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
base_value *
headers::validate(const std::string &name, const std::string &value)
{
	if (name == CONTENT_LENGTH) {
		return new num_single_val_t(value);
	} else if (name == TRANSFER_ENCODING) {
		return new tok_seq_val_t(value);
	} else if (name == TE) {
		return new tok_seq_val_t(value);
	} else if (name == TRAILERS) {
		return new str_seq_val_t(value);
	} else if (name == HOST) {
		return new hp_single_val_t(value);
	} else if (name == VIA) {
		return new vai_seq_val_t(value);
	} else if (name == CONNECTION) {
		return new cnxn_seq_val_t(value);
	} else if (name == CONTENT_TYPE) {
		return new mt_single_val_t(value);
	} else if (name == CONTENT_ENCODING) {
		return new ce_seq_val_t(value);
	} else if (name == CONTENT_LOCATION) {
		return new uri_single_val_t(value);
	} else if (name == DATE) {
		return new dt_single_val_t(value);
	} else {
		return new str_seq_val_t(value);
	}
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
	std::string name;
	std::string value;
	std::ostringstream oss;
	scanner scn{istr};

	if (!scn.read_token(name)) {
		throw bad_message("HTTP header name not found");
	}

	scn.read_opt_space();

	if (!scn.read_special(':')) {
		oss << "no \':\' after HTTP header (" << name << ")";
		throw bad_message(oss.str());
	}

	scn.read_opt_space();

	if (!scn.read_all(value)) {
		oss << "no HTTP header value for " << name;
		throw bad_message(oss.str());
	}

	scn.read_opt_space();

	if (!scn.read_crlf()) {
		oss << "HTTP header (" << name << " : " << name << ") not terminated with CRLF";
		throw bad_message(oss.str());
	}

	add(name, value);
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

	std::unique_ptr<base_value> v(validate(n, value));
	if (v.get()) {
		hdr_vec_t::iterator I = find(n);
		if (I == m_headers.end()) {
			m_headers.push_back(std::make_pair(n, std::shared_ptr<base_value>(v.release())));
		} else if (v->is_seq()) {
			ce_seq_val_t    *tgt_ceseq = 0,   *src_ceseq = 0;
			str_seq_val_t   *tgt_strseq = 0,  *src_strseq = 0;
			tok_seq_val_t   *tgt_tokseq = 0,  *src_tokseq = 0;
			vai_seq_val_t   *tgt_viaseq = 0,  *src_viaseq = 0;
			cnxn_seq_val_t  *tgt_cnxnseq = 0, *src_cnxnseq = 0;

			base_value *tgt_bv = I->second.get();
			base_value *src_bv = v.get();

			if ((tgt_ceseq = dynamic_cast<ce_seq_val_t *>(tgt_bv)) != 0) {
				src_ceseq = dynamic_cast<ce_seq_val_t *>(src_bv);
				if (src_ceseq)
					*tgt_ceseq += *src_ceseq;
			} else if ((tgt_strseq = dynamic_cast<str_seq_val_t *>(tgt_bv)) != 0) {
				src_strseq = dynamic_cast<str_seq_val_t *>(src_bv);
				if (src_strseq)
					*tgt_strseq += *src_strseq;
			} else if ((tgt_tokseq = dynamic_cast<tok_seq_val_t *>(tgt_bv)) != 0) {
				src_tokseq = dynamic_cast<tok_seq_val_t *>(src_bv);
				if (src_tokseq)
					*tgt_tokseq += *src_tokseq;
			} else if ((tgt_viaseq = dynamic_cast<vai_seq_val_t *>(tgt_bv)) != 0) {
				src_viaseq = dynamic_cast<vai_seq_val_t *>(src_bv);
				if (src_viaseq)
					*tgt_viaseq += *src_viaseq;
			} else if ((tgt_cnxnseq = dynamic_cast<cnxn_seq_val_t *>(tgt_bv)) != 0) {
				src_cnxnseq = dynamic_cast<cnxn_seq_val_t *>(src_bv);
				if (src_cnxnseq)
					*tgt_cnxnseq += *src_cnxnseq;
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

	base_value *v = validate(n, value);
	if (v) {
		hdr_vec_t::iterator I = find(n);
		if (I == m_headers.end()) {
			m_headers.push_back(std::make_pair(n, std::shared_ptr<base_value>(v)));
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
headers::update(const std::string &name, base_value *value)
{
	if (name.empty())
		return;

	std::string n;
	std::transform(name.begin(), name.end(), std::back_inserter(n),
		[] (char c) { return std::tolower(c); });

	hdr_vec_t::iterator I = find(n);
	if (I == m_headers.end()) {
		m_headers.push_back(std::make_pair(name, std::shared_ptr<base_value>(value)));
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
const base_value *
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
	const num_single_val_t *numval = dynamic_cast<const num_single_val_t *>(get(CONTENT_LENGTH));
	return numval->get();
}

void
headers::content_length(size_t length)
{
	update(CONTENT_LENGTH, new num_single_val_t(length));
}

void
headers::content_length(const std::string &str)
{
	update(CONTENT_LENGTH, new num_single_val_t(str));
}

const std::vector<token> &
headers::transfer_encoding() const
{
	const tok_seq_val_t *tseq = dynamic_cast<const tok_seq_val_t *>(get(TRANSFER_ENCODING));
	return tseq->get();
}

void
headers::transfer_encoding(const token &token)
{
	update(TRANSFER_ENCODING, new tok_seq_val_t(token));
}

void
headers::transfer_encoding(const std::vector<token> &tokens)
{
	update(TRANSFER_ENCODING, new tok_seq_val_t(tokens));
}

void
headers::transfer_encoding(const std::string &coding)
{
	update(TRANSFER_ENCODING, new tok_seq_val_t(coding));
}

bool
headers::is_message_chunked() const
{
	hdr_vec_t::const_iterator it = find(TRANSFER_ENCODING);
	if (it != m_headers.end()) {
		const tok_seq_val_t *tokseq = dynamic_cast<const tok_seq_val_t *>(it->second.get());
		if (tokseq) {
			for (auto it = tokseq->cbegin(); it != tokseq->cend(); ++it) {
				if (it->name == TRANSFER_ENCODING_CHUNKED)
					return true;
			}
		}
	}
	return false;
}

const std::vector<token> &
headers::te() const
{
	const tok_seq_val_t *tokseq = dynamic_cast<const tok_seq_val_t *>(get(TE));
	return tokseq->get();
}

void
headers::te(const token &token)
{
	update(TE, new tok_seq_val_t(token));
}

void
headers::te(const std::vector<token> &tokens)
{
	update(TE, new tok_seq_val_t(tokens));
}

void
headers::te(const std::string &coding)
{
	update(TE, new tok_seq_val_t(coding));
}

bool
headers::has_trailers() const
{
	hdr_vec_t::const_iterator it = find(TE);
	if (it != m_headers.end()) {
		const tok_seq_val_t *tokseq = dynamic_cast<const tok_seq_val_t *>(it->second.get());
		if (tokseq) {
			for (auto it = tokseq->cbegin(); it != tokseq->cend(); ++it) {
				if (it->name == TRAILERS)
					return true;
			}
		}
	}
	return false;
}

const std::vector<std::string> &
headers::trailers() const
{
	const str_seq_val_t *strseq = dynamic_cast<const str_seq_val_t *>(get(TRAILERS));
	return strseq->get();
}

void
headers::trailers(const std::string &field)
{
	update(TRAILERS, new str_seq_val_t(field));
}

void
headers::trailers(const std::vector<std::string> &fields)
{
	update(TRAILERS, new str_seq_val_t(fields));
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
	const hp_single_val_t *hpval = dynamic_cast<const hp_single_val_t *>(get(HOST));
	if (port)
		*port = hpval->get().port;
	return hpval->get().host;
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
	host_port hp;

	parse(hp, uristr);
	if (port)
		hp.port = port;

	update(HOST, new hp_single_val_t(hp));
}

const std::vector<via> &
headers::intermediary() const
{
	const vai_seq_val_t *vlist = dynamic_cast<const vai_seq_val_t *>(get(VIA));
	return vlist->get();
}

void
headers::intermediary(const via &v)
{
	update(VIA, new vai_seq_val_t(v));
}

void
headers::intermediary(const std::vector<via> &vvec)
{
	update(VIA, new vai_seq_val_t(vvec));
}

void
headers::intermediary(const std::string &viastr)
{
	update(VIA, new vai_seq_val_t(viastr));
}

const std::vector<std::string> &
headers::connection() const
{
	const cnxn_seq_val_t *cnxnseq = dynamic_cast<const cnxn_seq_val_t *>(get(CONNECTION));
	return cnxnseq->get();
}

void
headers::connection(const std::string &cnxn)
{
	update(CONNECTION, new cnxn_seq_val_t(cnxn));
}

void
headers::connection(const std::vector<std::string> &cnxns)
{
	update(CONNECTION, new cnxn_seq_val_t(cnxns));
}

const media_type &
headers::content_type() const
{
	const mt_single_val_t *mt = dynamic_cast<const mt_single_val_t *>(get(CONTENT_TYPE));
	return mt->get();
}

void
headers::content_type(const media_type &mt)
{
	update(CONTENT_TYPE, new mt_single_val_t(mt));
}

void
headers::content_type(const std::string &type, const std::string &subtype)
{
	update(CONTENT_TYPE, new mt_single_val_t(media_type(type, subtype)));
}

const std::vector<std::string> &
headers::content_encoding() const
{
	const ce_seq_val_t *ceseq = dynamic_cast<const ce_seq_val_t *>(get(CONTENT_ENCODING));
	return ceseq->get();
}

void
headers::content_encoding(const std::string &coding)
{
	update(CONTENT_ENCODING, coding);
}

const std::vector<std::string> &
headers::content_language() const
{
	const str_seq_val_t *strseq = dynamic_cast<const str_seq_val_t *>(get(CONTENT_LANGUAGE));
	return strseq->get();
}

void
headers::content_language(const std::string &language)
{
	update(CONTENT_LANGUAGE, new str_seq_val_t(language));
}

void
headers::content_language(const std::vector<std::string> &languages)
{
	update(CONTENT_LANGUAGE, new str_seq_val_t(languages));
}

uri
headers::content_location() const
{
	const uri_single_val_t *urival = dynamic_cast<const uri_single_val_t *>(get(CONTENT_LOCATION));
	return urival->get();
}

void
headers::content_location(const uri &u)
{
	update(CONTENT_LOCATION, new uri_single_val_t(u));
}

void
headers::content_location(const std::string &uristr)
{
	update(CONTENT_LOCATION, new uri_single_val_t(uri(uristr)));
}

const snf::datetime &
headers::date() const
{
	const dt_single_val_t *dtval = dynamic_cast<const dt_single_val_t *>(get(DATE));
	return dtval->get();
}

void
headers::date(time_t t)
{
	snf::datetime dt(t);
	update(DATE, new dt_single_val_t(dt));
}

void
headers::date(const snf::datetime &dt)
{
	update(DATE, new dt_single_val_t(dt));
}

void
headers::date(const std::string &dtstr)
{
	update(DATE, new dt_single_val_t(dtstr));
}

} // namespace http
} // namespace snf
