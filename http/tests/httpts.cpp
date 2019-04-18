#include "test.h"
#include "testmain.h"
#include "uritest.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW uritest(),
	0
};

} // namespace tf
} // namespace snf
