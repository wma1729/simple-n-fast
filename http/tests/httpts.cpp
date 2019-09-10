#include "test.h"
#include "testmain.h"
#include "uritest.h"
#include "rqstresp.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW uritest(),
	DBG_NEW rqstresp(),
	0
};

} // namespace tf
} // namespace snf
