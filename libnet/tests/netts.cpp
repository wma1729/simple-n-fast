#include "test.h"
#include "testmain.h"
#include "sockattr.h"
#include "privkey.h"
#include "certificate.h"

namespace snf {
namespace tf {

test *test_list[] = {
	DBG_NEW sock_attr(),
	DBG_NEW priv_key(),
	DBG_NEW certificate(),
	0
};

} // namespace tf
} // namespace snf
