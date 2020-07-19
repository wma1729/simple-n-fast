#ifndef _SNF_HTTP_ROUTER_H_
#define _SNF_HTTP_ROUTER_H_

#include <string>
#include <map>
#include <deque>
#include <stack>
#include <regex>
#include <mutex>
#include <functional>
#include "transmit.h"

namespace snf {
namespace http {

using request_handler_t = std::function<response(const request &)>;

struct path_segment;

using path_segments_t = std::deque<path_segment *>;
using path_elements_t = std::vector<std::string>;

/*
 * The path segments represents a single path element. It is a node in the
 * path tree.
 *
 * path1 = /element_1/element_2/{path_param}/element_3
 * path2 = /element_1/{path_param=<regexp>}/element_3
 *
 * path1 has the following path elements:
 * - element_1
 * - element_2
 * - {path_param}
 * - element_3
 * and the following path segments:
 * - segment.m_name = element_1, segment.m_param is empty, segment.m_regexpr = nullptr
 * - segment.m_name = element_2, segment.m_param is empty, segment.m_regexpr = nullptr
 * - segment.m_name = [^/]+, segment.m_param = path_param, segment.m_regexpr = new std::regex([^/]+)
 * - segment.m_name = element_3, segment.m_param is empty, segment.m_regexpr = nullptr
 *
 * path2 has the following elements:
 * - element_1
 * - {path_param=<regexp>}
 * - element_3
 * and the following path segments:
 * - segment.m_name = element_1, segment.m_param is empty, segment.m_regexpr = nullptr
 * - segment.m_name = <regexp>, segment.m_param = path_param, egment.m_regexpr = new std::regex(<regexp>)
 * - segment.m_name = element_3, segment.m_param is empty, segment.m_regexpr = nullptr
 */
struct path_segment
{
	std::string         m_name;     // segment (or regular expression string)
	std::string         m_param;    // parameter name
	std::regex          *m_regexpr; // regular expression object
	path_segments_t     m_children; // children segment
	request_handler_t   m_handler;  // request handler

	path_segment(const std::string &);
	~path_segment();
	bool is_param() const { return !m_param.empty(); }
	bool matches(const std::string &) const;
};

/*
 * Router routes requests to user-registered handlers.
 * It maintains a multi-rooted tree to hold the paths.
 * - p1/p2/p3, handler = h1
 * - p1/p2,    handler = h2
 * - p4/p5/p6, handler = h3
 * - p4/p5/p7, handler = h4
 *
 *      p1 [handler = nullptr]   ----   p4 [handler = nullptr]
 *      |                               |
 *      + p2 [handler = h2]             + p5 [handler = nullptr]
 *        |                               |
 *        + p3 [handler = h1]             + p6 [handler = h3]
 *          |                               |
 *          + nullptr                       + nullptr
 *                                        + p7 [handler = h4]
 *                                          |
 *                                          + nullptr
 * Incoming request             Action
 * =========================================================
 * p1                           No handler (not implemented)
 * p1/p2                        h2
 * p2/p2/p3                     h1
 * p4                           No handler (not implemented)
 * p4/p5                        No handler (not implemented)
 * p4/p5/p6                     h3
 * p4/p5/p7                     h4
 */
class router
{
private:
	path_segments_t m_root; // root of the path segment tree
	std::mutex      m_lock;

	router() {}

	path_elements_t split(const std::string &);
	path_segment *find(path_segments_t *, const std::string &, bool lookup = false);
	int handle(const path_segment *&, const path_segments_t *, path_elements_t::const_iterator,
		path_elements_t::const_iterator, std::stack<path_param_t> &param_stk);

public:
	~router()
	{
		std::lock_guard<std::mutex> guard(m_lock);

		path_segments_t::iterator it;
		for (it = m_root.begin(); it != m_root.end(); ++it) {
			path_segment *seg = *it;
			delete seg;
		}
		m_root.clear();
	}

	router(const router &) = delete;
	router(router &&) = delete;

	const router &operator=(const router &) = delete;
	router &operator=(router &&) = delete;

	static router & instance()
	{
		static router the_router;
		return the_router;
	}

	void add(const std::string &, request_handler_t);
	response handle(request &);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_ROUTER_H_
