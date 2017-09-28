#include "tf/testmain.h"
#include "tf/testmacros.h"
#include "simpleSGR.h"
#include "multipleKPN.h"
#include "normalFD.h"
#include "bigload.h"

static int
RandomInRange(unsigned int seed, int lo, int hi)
{
	if (lo == hi)
		return lo;

	if (seed != 0)
		srand(seed);

	return lo + (rand() % (hi - lo));
}

void
GenKeyValue(char *key, char *val, int len)
{
	int idx;
	char validChars[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f',
		'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r',
		's', 't', 'u', 'v',
		'w', 'x', 'y', 'z'
	};

	for (int i = 0; i < len; i++) {
		idx = RandomInRange(0, 0, 36);
		key[i] = validChars[idx];
		val[len - i - 1] = toupper(key[i]);
	}

	key[len] = '\0';
	val[len] = '\0';

	return;
}

namespace tf {

Test *TestList[] = {
	DBG_NEW SimpleSetGetRemove(),
	DBG_NEW MultipleKeyPageNodes(),
	DBG_NEW NormalFairDistribution(),
	DBG_NEW BigLoad(),
	0
};

}
