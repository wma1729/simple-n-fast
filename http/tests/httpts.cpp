#include "test.h"
#include "testmain.h"
#include "uritest.h"
#include "rqstresp.h"
#include "bodytest.h"
#include "routertest.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW uritest(),
	DBG_NEW rqstresp(),
	DBG_NEW bodytest(),
	DBG_NEW routertest(),
	0
};

} // namespace tf
} // namespace snf
