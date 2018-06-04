#include "test.h"
#include "testmain.h"
#include "nullvalue.h"
#include "boolvalue.h"
#include "intvalue.h"
#include "realvalue.h"
#include "strvalue.h"
#include "genvalue.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW null_value(),
	DBG_NEW bool_value(),
	DBG_NEW int_value(),
	DBG_NEW real_value(),
	DBG_NEW str_value(),
	DBG_NEW gen_value(),
	0
};

} // namespace tf
} // namespace snf
