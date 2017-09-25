#include "tf/testmain.h"
#include "tf/testassert.h"
#include "simpleSGR.h"

namespace tf {

Test *TestList[] = {
	new SimpleSetGetRemove(),
	0
};

}
