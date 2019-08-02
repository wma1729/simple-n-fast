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

	if (m_type == T_TEXT) {
		if (m_subtype != ST_PLAIN) {
			oss << "subtype " << m_subtype << " is not supported for type " << T_TEXT;
			throw not_implemented(oss.str());
		}
	} else if (m_type == T_APPLICATION) {
		if (m_subtype != ST_JSON) {
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
	size_t i;
	size_t len = istr.size();
	std::string type;
	std::string subtype;

	for (i = 0; i < len; ++i) {
		if (is_tchar(istr[i]))
			type.push_back(std::tolower(istr[i]));
		else if (istr[i] == '/')
			break;
		else
			throw bad_message("invalid character in type");
	}

	if (type.empty())
		throw bad_message("no type found");
	else if (istr[i] != '/')
		throw bad_message("type is not followed by /");
	else
		i++;

	for (; i < len; ++i) {
		if (is_tchar(istr[i]))
			subtype.push_back(std::tolower(istr[i]));
		else if ((istr[i] == ';') || is_whitespace(istr[i]))
			break;
		else
			throw bad_message("invalid character in subtype");
	}

	if (subtype.empty())
		throw bad_message("no subtype found");

	m_type = std::move(type);
	m_subtype = std::move(subtype);

	std::string name;
	std::string value;
	std::ostringstream oss;

	while (i < len) {
		while ((i < len) && is_whitespace(istr[i]))
			i++;

		if (i >= len)
			break;

		if (istr[i] == ';')
			i++;

		while ((i < len) && is_whitespace(istr[i]))
			i++;

		if (i >= len)
			break;

		bool processing_name = true;
		bool quoted = false;

		for (; i < len; ++i) {
			if (is_tchar(istr[i])) {
				if (processing_name)
					name.push_back(std::tolower(istr[i]));
				else
					value.push_back(istr[i]);
			} else if (istr[i] == '=') {
				i++;
				processing_name = false;
			} else if ((istr[i] == '"') && !processing_name) {
				quoted = !quoted;
				if (!quoted) {
					i++;
					break;
				}
			} else {
				oss << "invalid character in parameter "
					<< (processing_name ? "name" : "value");
				throw bad_message(oss.str());
			}
		}

		if (quoted) {
			oss << "parameter value does not terminate with \" for " << name;
			throw bad_message(oss.str());
		}

		if (!name.empty()) {
			if (value.empty()) {
				oss << "parameter value is not specified for " << name;
				throw bad_message(oss.str());
			} else {
				param(name, value);
			}
		} else {
			throw bad_message("parameter name is empty");
		}

		name.clear();
		value.clear();
	}

	validate();
}

std::ostream &
operator<< (std::ostream &os, const media_type &mt)
{
	os << mt.type() << "/" << mt.subtype();

	bool first = true;
	for (auto elem : mt.param()) {
		if (!first)
			os << ",";
		os << elem.first << "=" << elem.second;
		first = false;
	}

	return os;
}

} // namespace http
} // namespace snf
