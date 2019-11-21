#ifndef _SNF_HTTP_ROUTER_H_
#define _SNF_HTTP_ROUTER_H_

#include <string>
#include <map>
#include <deque>
#include <regex>
#include <functional>
#include "transmit.h"

namespace snf {
namespace http {

using request_handler = std::function<response(const request &)>;

struct path_segment;

using path_segments = std::deque<path_segment *>;

struct path_segment
{
	std::string     m_name;     // segment (or regular expression string)
	std::string     m_param;    // parameter name
	std::regex      *m_regexpr; // regular expression object
	path_segments   m_children; // children segment
	request_handler m_handler; // request handler

	path_segment(const std::string &);
	~path_segment();
	bool is_param() const { return !m_param.empty(); }
	bool matches(const std::string &) const;
};

using request_parameters = std::map<std::string, std::string>;

class router
{
private:
	path_segments   m_toplevel;

	router() {}

	std::vector<std::string> split(const std::string &);
	path_segment *find(path_segments &, const std::string &, bool lookup = false);

public:
	router(const router &) = delete;
	router(router &&) = delete;

	const router &operator=(const router &) = delete;
	router &operator=(router &&) = delete;

	static router &instance()
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
