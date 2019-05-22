#ifndef _SNF_HTTP_SERVER_CONFIG_H_
#define _SNF_HTTP_SERVER_CONFIG_H_

#include "net.h"
#include "cmncfg.h"

namespace snf {
namespace http {

class server_config : public common_config
{
private:
	int         m_nthreads = 20;    // default worker threads
	
public:
	server_config() : common_config() {}
	virtual ~server_config() {}

	int worker_thread_count() const { return m_nthreads; }
	void worker_thread_count(int n) { m_nthreads = n; }
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_SERVER_CONFIG_H_
