#include "router.h"
#include <ostream>
#include <sstream>
#include <stdexcept>
#include "charset.h"
#include "scanner.h"

namespace snf {
namespace http {

path_segment::path_segment(const std::string &seg)
	: m_name(seg)
	, m_regexpr(nullptr)
{
	if ((seg.front() == '{') && (seg.back() == '}')) {
		scanner scn{seg, 1, seg.length() - 1};
		if (!scn.read_path_parameter(m_param, m_name)) {
			std::ostringstream oss;
			oss << "invalid request paramter specified in " << seg;
			throw std::invalid_argument(oss.str());
		}

		try {
			m_regexpr = new std::regex(
					m_name,
					std::regex::ECMAScript |
					std::regex::icase |
					std::regex::optimize);

		} catch (const std::regex_error &ex) {
			std::ostringstream oss;
			oss << "invalid regular expression: " << ex.what();
			throw std::invalid_argument(oss.str());
		}
	} else {
		for (size_t i = 0; i < m_name.length(); ++i) {
			if (!uri_unreserved_character(m_name[i]) &&
				!uri_subcomponent_delimiter(m_name[i]) &&
				(m_name[i] != '@') &&
				(m_name[i] != ':')) {
				std::ostringstream oss;
				oss << "invalid character \'" << m_name[i]
					<< "\' in path segment " << m_name;
				throw std::invalid_argument(oss.str());
			}
		}
	}
}

path_segment::~path_segment()
{
	path_segments::iterator it;
	for (it = m_children.begin(); it != m_children.end(); ++it) {
		path_segment *seg = *it;
		delete seg;
	}
	m_children.clear();

	if (m_regexpr) {
		delete m_regexpr;
		m_regexpr = nullptr;
	}
}

bool
path_segment::matches(const std::string &elem) const
{
	if (m_regexpr)
		return std::regex_match(elem, *m_regexpr);
	return (elem == m_name);
}

path_elements
router::split(const std::string &path)
{
	size_t i = 0;
	size_t len = path.length();
	std::string elem;
	path_elements elements;

	while (i < len) {
		while ((i < len) && (path[i] == '/'))
			++i;

		while ((i < len) && (path[i] != '/')) {
			elem.push_back(path[i]);
			++i;
		}

		if (!elem.empty()) {
			elements.push_back(elem);
			elem.clear();
		}
	}

	return elements;
}

path_segment *
router::find(path_segments *segments, const std::string &name, bool lookup)
{
	path_segments::const_iterator it;
	for (it = segments->begin(); it != segments->end(); ++it) {
		path_segment *seg = *it;
		if (lookup) {
			if (seg->matches(name))
				return seg;
		} else {
			if (seg->m_name == name)
				return seg;
		}
	}
	return nullptr;
}

void
router::add(const std::string &path, request_handler handler)
{
	path_segment *seg = nullptr;
	path_segments *segments = &m_root;
	path_elements elements = std::move(split(path));

	std::lock_guard<std::mutex> guard(m_lock);

	path_elements::const_iterator it = elements.begin();
	while (it != elements.end()) {
		seg = find(segments, *it);
		if (seg == nullptr) {
			break;
		} else {
			segments = &(seg->m_children);
			++it;
		}
	}

	while (it != elements.end()) {
		seg = new path_segment(*it);

		if (seg->m_regexpr) {
			segments->push_back(seg);
		} else {
			segments->push_front(seg);
		}

		segments = &(seg->m_children);
		++it;
	}

	if (seg)
		seg->m_handler = std::move(handler);
}

int
router::handle(
	const path_segment *& seg,
	const path_segments *segments,
	path_elements::const_iterator curr,
	path_elements::const_iterator last,
	std::stack<path_param_t> &param_stk)
{
	int retval = E_not_found;

	if (curr == last)
		return E_ok;

	path_segments::const_iterator it;
	for (it = segments->begin(); it != segments->end(); ++it) {
		seg = *it;

		if (seg->matches(*curr)) {
			if (seg->is_param())
				param_stk.push(std::make_pair(seg->m_name, *curr));

			retval = handle(seg, &(seg->m_children), curr + 1, last, param_stk);
			if (E_ok == retval)
				break;

			if (seg->is_param())
				param_stk.pop();
		}
	}

	return retval;
}

response
router::handle(request &req)
{
	int retval = E_ok;
	std::string path = std::move(req.get_uri().get_path().get());
	path_elements elements = std::move(split(path));
	const path_segment *seg = nullptr;
	const path_segments *segments = &m_root;
	request_handler handler;

	{
		std::stack<path_param_t> param_stk;

		std::lock_guard<std::mutex> guard(m_lock);

		retval = handle(seg, segments, elements.begin(), elements.end(), param_stk);
		if (E_ok == retval) {
			if (seg) {
				handler = seg->m_handler;
			}

			while (!param_stk.empty()) {
				req.set_path_parameter(param_stk.top());
				param_stk.pop();
			}
		}
	}

	std::ostringstream oss;

	if (E_ok == retval) {
		try {
			return (handler)(req);
		} catch (std::bad_function_call &) {
			oss << method(req.get_method())
				<< " is not implemented for resource ("
				<< req.get_uri() << ")";
			throw not_implemented(oss.str());
		}
	} else {
		oss << "resource (" << req.get_uri() << ") is not found";
		throw not_found(oss.str());
	}
}

} // namespace http
} // namespace snf
