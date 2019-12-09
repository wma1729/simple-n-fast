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

using request_handler = std::function<response(const request &)>;

struct path_segment;

using path_segments = std::deque<path_segment *>;
using path_elements = std::vector<std::string>;
using rqst_param    = std::pair<std::string, std::string>;

struct path_segment
{
	std::string     m_name;     // segment (or regular expression string)
	std::string     m_param;    // parameter name
	std::regex      *m_regexpr; // regular expression object
	path_segments   m_children; // children segment
	request_handler m_handler;  // request handler

	path_segment(const std::string &);
	~path_segment();
	bool is_param() const { return !m_param.empty(); }
	bool matches(const std::string &) const;
};

class router
{
private:
	path_segments   m_toplevel;
	std::mutex      m_lock;

	router() {}

	path_elements split(const std::string &);
	path_segment *find(path_segments *, const std::string &, bool lookup = false);
	int handle(const path_segment *&, const path_segments *, path_elements::const_iterator,
		path_elements::const_iterator, std::stack<rqst_param> &param_stk);

public:
	~router()
	{
		std::lock_guard<std::mutex> guard(m_lock);

		path_segments::iterator it;
		for (it = m_toplevel.begin(); it != m_toplevel.end(); ++it) {
			path_segment *seg = *it;
			delete seg;
		}
		m_toplevel.clear();
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

	void add(const std::string &, request_handler h);
	response handle(request &);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_ROUTER_H_
