#include "server.h"
#include "router.h"
#include "srvrcfg.h"
#include "error.h"

static void
get_server_config(snf::http::server_config &srvrcfg)
{
	srvrcfg.http_port(8080);
	srvrcfg.https_port(8443);
	srvrcfg.keyfile("/home/moji/snf/http/tests/server/server.key.pem");
	srvrcfg.keyfile_password("Wma;1729");
	srvrcfg.certfile("/home/moji/snf/http/tests/server/server.cert.pem");
	srvrcfg.cafile("/home/moji/snf/http/tests/server/chain.cert.pem");
	srvrcfg.certificate_chain_depth(2);
}

int
main(int, const char **)
{
	snf::http::server_config srvrcfg;
	get_server_config(srvrcfg);

	snf::http::server &srvr = snf::http::server::instance();
	srvr.start(&srvrcfg);
	sleep(10);
	srvr.stop();
	return E_ok;
}
