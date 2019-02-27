#include "uri.h"
#include "ia.h"
#include <limits>
#include <sstream>
#include <iomanip>

namespace snf {
namespace net {

/*
 * Generic % encode string. Can be used directly
 * with query and fragment components.
 */
std::string
uri_component::encode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if ((':' == istr[i]) || ('@' == istr[i]) ||
			('/' == istr[i]) || ('?' == istr[i])) {
			ostr.push_back(istr[i]);
		} else {
			std::stringstream ss;
			ss
				<< '%'
				<< std::setfill('0')
				<< std::setw(2)
				<< std::setiosflags(
					std::ios_base::hex |
					std::ios_base::uppercase)
				<< static_cast<int>(istr[i]);
			ostr.append(ss.str());
		}
	}

	return ostr;
}

/*
 * Generic % decode string. Can be used directly
 * with query and fragment components.
 */
std::string
uri_component::decode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if ((':' == istr[i]) || ('@' == istr[i]) ||
			('/' == istr[i]) || ('?' == istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_percent_encoded(istr, i)) {
			std::string pct_encoded = istr.substr(i + 1, 2);
			char c = static_cast<char>(std::stoi(pct_encoded, 0, 16));
			ostr.push_back(c);
			i += 2;
		} else {
			throw std::runtime_error("invalid component: " + istr);
		}
	}

	return ostr;
}

/*
 * Is URI scheme valid?
 * scheme = [a-zA-Z][a-zA-Z0-9+-\.]*
 */
bool
uri_scheme::is_valid(const std::string &scheme) const
{
	if (scheme.empty())
		return false;

	if (!std::isalpha(scheme[0]))
		return false;

	bool valid = true;

	for (size_t i = 1; i < scheme.size(); ++i) {
		valid = std::isalpha(scheme[i]) || std::isdigit(scheme[i]) ||
			(scheme[i] == '+') || (scheme[i] == '-') || (scheme[i] == '.');
		if (!valid)
			break;
	}

	return valid;
}

/*
 * Set scheme.
 *
 * @param [in] scheme - scheme name.
 *
 * @throws std::runtime_error if the scheme name is invalid.
 */
void
uri_scheme::set(const std::string &scheme)
{
	if (!is_valid(scheme))
		throw std::runtime_error("invalid scheme: " + scheme);

	m_component = scheme;
}

/*
 * Is user info valid?
 * userinfo = [(unreserved)|(percent-encoded)|(subcomponent-delimiter)|:]*
 */
bool
uri_userinfo::is_valid(const std::string &ui) const
{
	if (ui.empty())
		return false;

	bool valid = true;

	for (size_t i = 0; i < ui.size(); ++i) {
		valid = uri_unreserved_character(ui[i]) ||
			uri_subcomponent_delimiter(ui[i]) ||
			(ui[i] == ':');

		if (!valid) {
			if (uri_percent_encoded(ui, i)) {
				i += 2;
				valid = true;
			} else {
				break;
			}
		}
	}

	return valid;
}

/*
 * Encode user info.
 */
std::string
uri_userinfo::encode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (':' == istr[i]) {
			ostr.push_back(istr[i]);
		} else {
			std::stringstream ss;
			ss
				<< '%'
				<< std::setfill('0')
				<< std::setw(2)
				<< std::setiosflags(
					std::ios_base::hex |
					std::ios_base::uppercase)
				<< static_cast<int>(istr[i]);
			ostr.append(ss.str());
		}
	}

	return ostr;
}

/*
 * Decode user info.
 */
std::string
uri_userinfo::decode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (':' == istr[i]) {
			ostr.push_back(istr[i]);
		} else if (uri_percent_encoded(istr, i)) {
			std::string pct_encoded = istr.substr(i + 1, 2);
			char c = static_cast<char>(std::stoi(pct_encoded, 0, 16));
			ostr.push_back(c);
			i += 2;
		} else {
			throw std::runtime_error("invalid userinfo: " + istr);
		}
	}

	return ostr;
}

/*
 * Is host name valid?
 * reghostname = [(unreserved)|(percent-encoded)|(subcomponent-delimiter)]*
 */
bool
uri_host::is_regular_hostname(const std::string &host) const
{
	if (host.empty())
		return false;

	bool valid = true;

	for (size_t i = 0; i < host.size(); ++i) {
		valid = uri_unreserved_character(host[i]) ||
			uri_subcomponent_delimiter(host[i]);

		if (!valid) {
			if (uri_percent_encoded(host, i))
				i += 2;
			else
				break;
		}
	}

	return valid;
}

/*
 * Is a valid host name?
 * hostname = [(\[(IPv6-addr)\])|(IPv4-addr)|(reg-host-name)]
 */
bool
uri_host::is_valid(const std::string &host) const
{
	if (host.empty())
		return false;

	bool valid = true;

	if (('[' == host.front()) && (']' == host.back())) {
		try {
			internet_address ia { AF_INET6, host.substr(1, host.size() - 2) };
		} catch (std::runtime_error &) {
			valid = false;
		}
	} else {
		try {
			internet_address ia { AF_INET, host };
		} catch (std::runtime_error &) {
			if (!is_regular_hostname(host))
				valid = false;
		}
	}

	return valid;
}

/*
 * Encode host.
 */
std::string
uri_host::encode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(std::tolower(istr[i]));
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if ((':' == istr[i]) || ('.' == istr[i]) ||
			('[' == istr[i]) || (']' == istr[i])) {
			ostr.push_back(istr[i]);
		} else {
			std::stringstream ss;
			ss
				<< '%'
				<< std::setfill('0')
				<< std::setw(2)
				<< std::setiosflags(
					std::ios_base::hex |
					std::ios_base::uppercase)
				<< static_cast<int>(istr[i]);
			ostr.append(ss.str());
		}
	}

	return ostr;
}

/*
 * Decode host.
 */
std::string
uri_host::decode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if ((':' == istr[i]) || ('.' == istr[i]) ||
			('[' == istr[i]) || (']' == istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_percent_encoded(istr, i)) {
			std::string pct_encoded = istr.substr(i + 1, 2);
			char c = static_cast<char>(std::stoi(pct_encoded, 0, 16));
			ostr.push_back(c);
			i += 2;
		} else {
			throw std::runtime_error("invalid host: " + istr);
		}
	}

	return ostr;
}

/*
 * Is valid port?
 * port = [0-9]*
 * valid range: 1 - 65535
 */
bool
uri_port::is_valid(const std::string &port) const
{
	if (port.empty())
		return false;

	bool valid = true;

	for (size_t i = 0; i < port.size(); ++i) {
		valid = std::isdigit(port[i]);
		if (!valid)
			break;
	}

	if (valid) {
		int p = std::stoi(port);
		if ((p <= 0) || (p > 0xFFFF))
			valid = false;
	}

	return valid;
}

/*
 * Set port.
 *
 * @param [in] port - port string.
 *
 * @throws std::runtime_error if the port is invalid.
 */
void
uri_port::set(const std::string &port)
{
	if (!is_valid(port))
		throw std::runtime_error("invalid port: " + port);

	m_component = port;
}

/*
 * Is valid path?
 * path = [(unreserved)|(subcomponent-delimiter)|(percent-encoded)|@|/|:]*
 * "//" not allowed.
 */
bool
uri_path::is_valid(const std::string &path) const
{
	if (path.empty())
		return true;

	if ((path.size() == 1) && (path[0] == '/'))
		return true;

	bool valid = true;
	int lc = -1;

	for (size_t i = 0; i < path.size(); ++i) {
		valid = uri_unreserved_character(path[i]) ||
			uri_subcomponent_delimiter(path[i]) ||
			(path[i] == '@') ||
			(path[i] == ':');

		if (!valid) {
			if ((path[i] == '/') && (lc != '/')) {
				valid = true;
			} else if (uri_percent_encoded(path, i)) {
				i += 2;
				valid = true;
			}
		}

		if (valid)
			lc = path[i];
		else
			break;
	}

	return valid;
}

/*
 * Encode path.
 */
std::string
uri_path::encode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (('@' == istr[i]) || (':' == istr[i]) || ('/' == istr[i])) {
			ostr.push_back(istr[i]);
		} else {
			std::stringstream ss;
			ss
				<< '%'
				<< std::setfill('0')
				<< std::setw(2)
				<< std::setiosflags(
					std::ios_base::hex |
					std::ios_base::uppercase)
				<< static_cast<int>(istr[i]);
			ostr.append(ss.str());
		}
	}

	return ostr;
}

/*
 * Decode path.
 */
std::string
uri_path::decode(const std::string &istr) const
{
	std::string ostr;

	for (size_t i = 0; i < istr.size(); ++i) {
		if (uri_unreserved_character(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_subcomponent_delimiter(istr[i])) {
			ostr.push_back(istr[i]);
		} else if (('@' == istr[i]) || (':' == istr[i]) || ('/' == istr[i])) {
			ostr.push_back(istr[i]);
		} else if (uri_percent_encoded(istr, i)) {
			std::string pct_encoded = istr.substr(i + 1, 2);
			char c = static_cast<char>(std::stoi(pct_encoded, 0, 16));
			ostr.push_back(c);
			i += 2;
		} else {
			throw std::runtime_error("invalid path: " + istr);
		}
	}

	return ostr;
}

/*
 * Converts segment vector to path string.
 *
 * @param [in] segments - path segments.
 *
 * @return path string.
 */
std::string
uri_path::segments_to_path(
	const std::deque<std::string> &segments,
	bool slash_at_start,
	bool slash_at_end) const
{
	std::string path;

	if (!segments.empty()) {
		std::ostringstream oss;
		int segnum = 0;

		if (slash_at_start)
			oss << "/";

		for (auto s : segments) {
			if (segnum)
				oss << "/";
			oss << s;
			segnum++;
		}

		if (segnum && slash_at_end)
			oss << "/";

		path = oss.str();
	}

	return path;
}

/*
 * Set path.
 *
 * @param [in] path - URL path.
 *
 * @throws std::runtime_error if the path is invalid.
 */
void
uri_path::set(const std::string &path)
{
	m_path = std::move(decode(path));
	std::string p = m_path;

	size_t si = 0;
	size_t ei;

	if ('/' == p.back())
		m_slash_at_end = true;

	if ('/' == p.front()) {
		m_slash_at_start = true;
		si = 1;
	}

	std::string segment;

	while (si < p.size()) {
		ei = p.find_first_of('/', si);
		if (ei != std::string::npos) {
			segment = p.substr(si, (ei - si));
			si = ei + 1;
		} else {
			segment = p.substr(si);
			si = p.size();
		}

		if (segment == "..") {
			if (m_slash_at_start) {
				if (!m_segments.empty())
					m_segments.pop_back();
			} else {
				m_segments.emplace_back(segment);
			}
		} else if (segment == ".") {
			if (!m_slash_at_start)
				m_segments.emplace_back(segment);
		} else {
			if (!segment.empty())
				m_segments.emplace_back(segment);
		}
	}

	if (!m_segments.empty())
		m_component = std::move(segments_to_path(
				m_segments, m_slash_at_start, m_slash_at_end));
}

/*
 * Merge a relative path into this URL path.
 *
 * @param [in] rel - relative URL path.
 *
 * @return a new URL path that is merge of base and relative paths.
 */
uri_path
uri_path::merge(const uri_path &rel) const
{
	uri_path target;

	if (rel.m_slash_at_start) {
		target = rel;
		return target;
	}

	std::deque<std::string> segments = m_segments;

	if (!segments.empty())
		segments.pop_back();

	for (auto s : rel.m_segments) {
		if (s == "..") {
			if (!segments.empty())
				segments.pop_back();
		} else if (s != ".") {
			segments.emplace_back(s);
		}
	}

	if (!segments.empty()) {
		std::string path = std::move(segments_to_path(
				segments, m_slash_at_start, rel.m_slash_at_end));
		target.set(path);
	}

	return target;
}

/*
 * Is valid query?
 * query = [(unreserved)|(percent-encoded)|(subcomponent-delimiter)|:|@|/|?]*
 */
bool
uri_query::is_valid(const std::string &query) const
{
	if (query.empty())
		return false;

	bool valid = true;

	for (size_t i = 0; i < query.size(); ++i) {
		valid = uri_unreserved_character(query[i]) ||
			uri_subcomponent_delimiter(query[i]) ||
			(query[i] == ':') ||
			(query[i] == '@') ||
			(query[i] == '/') ||
			(query[i] == '?');

		if (!valid) {
			if (uri_percent_encoded(query, i)) {
				i += 2;
				valid = true;
			} else {
				break;
			}
		}
	}

	return valid;
}

/*
 * Is valid fragment?
 * fragment = [(unreserved)|(percent-encoded)|(subcomponent-delimiter)|:|@|/|?]*
 */
bool
uri_fragment::is_valid(const std::string &frag) const
{
	if (frag.empty())
		return false;

	bool valid = true;

	for (size_t i = 0; i < frag.size(); ++i) {
		valid = uri_unreserved_character(frag[i]) ||
			uri_subcomponent_delimiter(frag[i]) ||
			(frag[i] == ':') ||
			(frag[i] == '@') ||
			(frag[i] == '/') ||
			(frag[i] == '?');

		if (!valid) {
			if (uri_percent_encoded(frag, i)) {
				i += 2;
				valid = true;
			} else {
				break;
			}
		}
	}

	return valid;
}

/*
 * Parse the scheme name. The scheme name ends at the next ':'.
 *
 * @param [in] str - URL string.
 * @param [in] si  - start index in the URL string.
 *
 * @return end index in the URL string where the next component begins.
 *
 * @throws std::runtime_error in case of error.
 */
size_t
uri::parse_scheme(const std::string &str, size_t si)
{
	size_t ei;

	for (ei = si; ei < str.size(); ++ei)
		if (str[ei] == ':')
			break;

	if (str[ei] != ':') {
		std::ostringstream oss;
		oss << "invalid scheme at index " << si;
		throw std::runtime_error(oss.str());
	}

	set_scheme(str.substr(si, (ei - si)));
	return ei + 1;
}

/*
 * Parse the authority. The authority ends at the next '/' or
 * '?' or '#' or the end of the URL string.
 *
 * @param [in] str - URL string.
 * @param [in] si  - start index in the URL string.
 *
 * @return end index in the URL string where the next component begins.
 *
 * @throws std::runtime_error in case of error.
 */
size_t
uri::parse_authority(const std::string &str, size_t si)
{
	if ((str[si] != '/') || (str[si + 1] != '/')) {
		std::ostringstream oss;
		oss << "invalid authority at index " << si;
		throw std::runtime_error(oss.str());
	}

	si += 2;

	size_t ei;

	for (ei = si; ei < str.size(); ++ei)
		if ((str[ei] == '/') || (str[ei] == '?') || (str[ei] == '#'))
			break;

	std::string auth = str.substr(si, (ei - si));

	size_t i = auth.find_first_of('@', 0);
	if (i != std::string::npos) {
		std::string userinfo = auth.substr(0, i);

		if (!m_userinfo.is_valid(userinfo))
			throw std::runtime_error("invalid userinfo: " + userinfo);

		set_userinfo(userinfo);
		++i;
	} else {
		i = 0;
	}

	std::string host;

	if (auth[i] == '[') {
		while (i < auth.size()) {
			host.push_back(auth[i]);
			if (auth[i] == ']')
				break;
			++i;
		}

		if (auth[i] != ']') {
			std::ostringstream oss;
			oss << "invalid host at index " << i;
			throw std::runtime_error(oss.str());
		} else {
			++i;
		}
	} else {
		while (i < auth.size()) {
			if (auth[i] == ':')
				break;
			else if (auth[i] == '/')
				break;
			else
				host.push_back(auth[i]);
			++i;
		}
	}

	if (!m_host.is_valid(host))
		throw std::runtime_error("invalid host: " + host);

	set_host(host);

	if (auth[i] == ':')
		set_port(auth.substr(i + 1));

	return ei;
}

/*
 * Parse the path. The path ends at the next '?' or '#' or
 * the end of the URL string.
 *
 * @param [in] str - URL string.
 * @param [in] si  - start index in the URL string.
 *
 * @return end index in the URL string where the next component begins.
 *
 * @throws std::runtime_error in case of error.
 */
size_t
uri::parse_path(const std::string &str, size_t si)
{
	size_t ei;

	for (ei = si; ei < str.size(); ++ei)
		if ((str[ei] == '?') || (str[ei] == '#'))
			break;

	std::string path = str.substr(si, (ei - si));

	if (!m_path.is_valid(path))
		throw std::runtime_error("invalid path 2: " + path);

	m_path.set(path);
	return ei;
}

/*
 * Parse the query. The query ends at the next '#' or
 * the end of the URL string.
 *
 * @param [in] str - URL string.
 * @param [in] si  - start index in the URL string.
 *
 * @return end index in the URL string where the next component begins.
 *
 * @throws std::runtime_error in case of error.
 */
size_t
uri::parse_query(const std::string &str, size_t si)
{
	if (str[si] != '?') {
		std::ostringstream oss;
		oss << "invalid query at index " << si;
		throw std::runtime_error(oss.str());
	}

	si++;

	size_t ei;

	for (ei = si; ei < str.size(); ++ei)
		if (str[ei] == '#')
			break;

	std::string query = str.substr(si, (ei - si));
	if (!m_query.is_valid(query))
		throw std::runtime_error("invalid query: " + query);

	set_query(query);
	return ei;
}

/*
 * Parse the fragment. The query ends at the end of the URL string.
 *
 * @param [in] str - URL string.
 * @param [in] si  - start index in the URL string.
 *
 * @return end index in the URL string where the next component begins.
 *
 * @throws std::runtime_error in case of error.
 */
size_t
uri::parse_fragment(const std::string &str, size_t si)
{
	if (str[si] != '#') {
		std::ostringstream oss;
		oss << "invalid fragment at index " << si;
		throw std::runtime_error(oss.str());
	}

	si++;

	std::string fragment = str.substr(si);
	if (!m_fragment.is_valid(fragment))
		throw std::runtime_error("invalid fragment: " + fragment);

	set_fragment(fragment);
	return str.size();
}

/*
 * Parses the URI string. This is accomplished by breaking
 * the path into 5 components first:
 * - scheme
 * - authority ([userinfo]@host[:port])
 * - path
 * - query
 * - fragment.
 *
 * Each of these components are parsed independently then.
 *
 * @throws std::runtime_error in case of error.
 */
void
uri::parse(const std::string &str)
{
	size_t i = 0;
	bool scheme_seen = false;

	while (i < str.size()) {
		if (str[i] == '/') {
			if (str[i + 1] == '/')
				i = parse_authority(str, i);
			else
				i = parse_path(str, i);
		} else if (str[i] == '?') {
			i = parse_query(str, i);
		} else if (str[i] == '#') {
			i = parse_fragment(str, i);
		} else if (scheme_seen) {
			i = parse_path(str, i);
		} else {
			/*
			 * Now it is a bit tricky. The first element could either
			 * be the scheme or the relative path segment. But if there is
			 * a ':' before '/', it must be scheme. Lets figure this out.
			 */
			int c = '\0';
			for (size_t j = i; j < str.size(); ++j) {
				if (str[j] == ':') {
					c = str[j];
					break;
				} else if (str[j] == '/') {
					c = str[j];
					break;
				}
			}

			if (c == ':') {
				i = parse_scheme(str, i);
				scheme_seen = true;
			} else {
				i = parse_path(str, i);
			}
		}
	}
}

/*
 * Merge a relative URL with the base (this) URL.
 *
 * @param [in] rel - relative URL.
 *
 * @return the merged URL.
 */
uri
uri::merge(const uri &rel) const
{
	uri target;

	if (rel.m_scheme.is_present()) {
		target = rel;
	} else {
		target.m_scheme = m_scheme;

		if (rel.m_host.is_present()) {
			if (rel.m_userinfo.is_present())
				target.m_userinfo = rel.m_userinfo;

			target.m_host = rel.m_host;

			if (rel.m_port.is_present())
				target.m_port = rel.m_port;

			target.m_path = rel.m_path;
			target.m_query = rel.m_query;
		} else {
			target.m_userinfo = m_userinfo;
			target.m_host = m_host;
			target.m_port = m_port;

			if (rel.m_path.is_present()) {
				target.m_path = std::move(m_path.merge(rel.m_path));
				target.m_query = rel.m_query;
			} else {
				target.m_path = m_path;

				if (rel.m_query.is_present())
					target.m_query = rel.m_query;
				else
					target.m_query = m_query;
			}
		}

		target.m_fragment = rel.m_fragment;
	}

	return target;
}

} // namespace net
} // namespace snf
