#include "test.h"
#include "testmain.h"
#include "nullvalue.h"
#include "boolvalue.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW null_value(),
	DBG_NEW bool_value(),
	0
};

} // namespace tf
} // namespace snf
