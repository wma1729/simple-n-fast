#include "test.h"
#include "testmain.h"
#include "sockattr.h"
#include "privkey.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW sock_attr(),
	DBG_NEW priv_key(),
	0
};

} // namespace tf
} // namespace snf
