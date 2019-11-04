#include "test.h"
#include "testmain.h"
#include "tp.h"
#include "dttest.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW tp(),
	DBG_NEW dttest(),
	0
};

} // namespace tf
} // namespace snf
