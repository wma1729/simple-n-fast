#ifndef _SNF_TLS_H_
#define _SNF_TLS_H_

#include "sslfcn.h"
#include "crt.h"
#include "ctx.h"
#include "session.h"

namespace snf {
namespace ssl {

class tls {
private:
	bool    m_server = false;
	SSL     *m_ssl = nullptr;

public:
	tls(bool, context &);
	tls(const tls &);
	tls(tls &&);
	virtual ~tls();

	const tls &operator=(const tls &);
	tls &operator=(tls &&);

	void set_context(context &);
	void check_hosts(const std::vector<std::string> &);
	void check_inaddr(const char *);
	std::string get_sni();
	void set_sni(const std::string &);
	session get_session();
	void set_session(session &);
	bool is_session_reused();
	int get_socket();
	void set_socket(int);
	int connect();
	int accept();
	int read(char *, int, int *);
	int write(const char *, int, int *);
	int shutdown();
	void reset();
	x509_certificate *get_peer_certificate();
	bool is_verification_successful(std::string &);
	int get_error(int);
};

} // namespace ssl
} // namespace snf

#endif // _SNF_TLS_H_
