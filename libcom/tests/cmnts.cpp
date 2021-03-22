#include "test.h"
#include "testmain.h"
#include "tp.h"
#include "dttest.h"
#include "claptest.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW tp(),
	DBG_NEW dttest(),
	DBG_NEW claptest(),
	0
};

} // namespace tf
} // namespace snf
