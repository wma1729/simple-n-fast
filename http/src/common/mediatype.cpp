#include "common.h"
#include "mediatype.h"
#include "charset.h"
#include "status.h"
#include <ostream>
#include <sstream>

namespace snf {
namespace http {

/*
 * Validate the type/subtype.
 * Implemented values are:
 * - text/plain
 * - application/json
 *
 * @throws snf::http::not_implemented if the content
 *         type is not implemented.
 */
void
media_type::validate()
{
	std::ostringstream oss;

	if (snf::streq(m_type, T_TEXT)) {
		if (!snf::streq(m_subtype, ST_PLAIN)) {
			oss << "subtype " << m_subtype << " is not supported for type " << T_TEXT;
			throw not_implemented(oss.str());
		}
	} else if (snf::streq(m_type, T_APPLICATION)) {
		if (!snf::streq(m_subtype, ST_JSON)) {
			oss << "subtype " << m_subtype << " is not supported for type " << T_APPLICATION;
			throw not_implemented(oss.str());
		}
	} else if (!m_type.empty()) {
		oss << "type " << m_type << " is not supported";
		throw not_implemented(oss.str());
	}
}

/*
 * Parse the value of the Content-Type header field.
 *
 * @param [in] istr - value of the content type.
 *
 * @throws snf::http::bad_message if the value could
 *         not be parsed.
 *         snf::http::not_implemented if the content
 *         type is not implemented.
 */
void
media_type::parse(const std::string &istr)
{
	size_t i = 0;
	size_t len = istr.size();

	std::string type = parse_token(istr, i, len);

	if (type.empty())
		throw bad_message("no type found");
	else if (istr[i] != '/')
		throw bad_message("type is not followed by /");
	else
		i++;

	std::string subtype = parse_token(istr, i, len);

	if (subtype.empty())
		throw bad_message("no subtype found");
	else if ((i < len) && ((istr[i] == ';') || is_whitespace(istr[i])))
		m_parameters = std::move(parse_parameter(istr, i, len));
	else
		throw bad_message("invalid character after subtype");

	m_type = std::move(type);
	m_subtype = std::move(subtype);
	
	validate();
}

std::ostream &
operator<< (std::ostream &os, const media_type &mt)
{
	os << mt.type() << "/" << mt.subtype();
	for (auto elem : mt.param())
		os << ";" << elem.first << "=" << elem.second;
	return os;
}

} // namespace http
} // namespace snf
