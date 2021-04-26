#include "random.h"

namespace snf {
namespace ssl {

random::random()
{
	m_engine.seed(m_random());
}

void
random::bytes(safestr &ss, bool private_prng_instance)
{
	bytes(ss.data(), ss.length(), private_prng_instance);
}

void
random::bytes(uint8_t *buf, int num, bool private_prng_instance)
{
	int r;

	if (private_prng_instance) {
		r = CRYPTO_FCN<p_rand_bytes>("RAND_priv_bytes")(buf, num);
	} else {
		r = CRYPTO_FCN<p_rand_bytes>("RAND_bytes")(buf, num);
	}

	if (r != 1)
		throw exception("failed to generate random bytes");
}

} // namespace ssl
} // namespace snf
