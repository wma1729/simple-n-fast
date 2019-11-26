#include "router.h"
#include <ostream>
#include <sstream>
#include <stdexcept>
#include "parseutil.h"

namespace snf {
namespace http {

path_segment::path_segment(const std::string &seg)
	: m_name(seg)
	, m_regexpr(nullptr)
{
	if ((seg.front() == '{') && (seg.back() == '}')) {
		size_t i = 1;
		size_t len = seg.length() - 1;

		skip_spaces(seg, i, len);

		while ((i < len) && (seg[i] != ':')) {
			m_param.push_back(seg[i]);
			++i;
		}

		if (m_param.empty()) {
			std::ostringstream oss;
			oss << "no parameter name specified in " << seg;
			throw std::invalid_argument(oss.str());
		}

		skip_spaces(seg, i, len);

		m_name = std::move(seg.substr(i, len - i));
		if (m_name.empty()) {
			m_name = R"([^/]+)";
		} else {
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
path_segment::matches(const std::string &seg) const
{
	if (m_regexpr)
		return std::regex_match(seg, *m_regexpr);
	return (seg == m_name);
}

std::vector<std::string>
router::split(const std::string &path)
{
	size_t i = 0;
	size_t len = path.length();
	std::string seg;
	std::vector<std::string> segments;

	while (i < len) {
		while ((i < len) && (path[i] == '/'))
			++i;

		while ((i < len) && (path[i] != '/')) {
			seg.push_back(path[i]);
			++i;
		}

		if (!seg.empty()) {
			segments.push_back(seg);
			seg.clear();
		}
	}

	return segments;
}

path_segment *
router::find(path_segments &segments, const std::string &name, bool lookup)
{
	for (auto seg : segments) {
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
	path_segments &psegments = m_toplevel;

	std::vector<std::string> segments = std::move(split(path));

	std::vector<std::string>::const_iterator it;
	for (it = segments.begin(); it != segments.end(); ++it) {
		seg = find(psegments, *it);
		if (seg == nullptr) {
			break;
		} else {
			psegments = seg->m_children;
		}
	}

	while (it != segments.end()) {
		seg = new path_segment(*it);
		if (seg->m_regexpr)
			psegments.push_back(seg);
		else
			psegments.push_front(seg);

		psegments = seg->m_children;
		++it;
	}

	if (seg)
		seg->m_handler = std::move(handler);
}

response
router::handle(request &req)
{
	std::ostringstream oss;
	std::string path = std::move(req.get_uri().get_path().get());
	std::vector<std::string> segments = std::move(split(path));

	path_segment *seg = nullptr;
	path_segments &psegments = m_toplevel;

	std::vector<std::string>::const_iterator it;
	for (it = segments.begin(); it != segments.end(); ++it) {
		seg = find(psegments, *it, true);
		if (seg == nullptr) {
			oss << "resource (" << req.get_uri() << ") is not found";
			throw not_found(oss.str());
		} else {
			if (seg->is_param())
				req.set_parameter(seg->m_name, *it);
			psegments = seg->m_children;
		}
	}

	if (seg) {
		try {
			return (seg->m_handler)(req);
		} catch (std::bad_function_call &) {
			oss << method(req.get_method())
				<< " is not implemented for resource ("
				<< req.get_uri() << ")";
			throw not_implemented(oss.str());
		}
	}

	oss << "resource (" << req.get_uri() << ") is not found";
	throw not_found(oss.str());
}

} // namespace http
} // namespace snf
