#include "tf/testmain.h"
#include "tf/testmacros.h"
#include "simpleSGR.h"

namespace tf {

Test *TestList[] = {
	DBG_NEW SimpleSetGetRemove(),
	0
};

}
