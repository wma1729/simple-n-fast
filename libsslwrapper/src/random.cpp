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
random::bytes(uint8_t *buf, size_t num, bool private_prng_instance)
{
	int r;
	int n = static_cast<int>(num);

	if (private_prng_instance) {
		r = CRYPTO_FCN<p_rand_bytes>("RAND_priv_bytes")(buf, n);
	} else {
		r = CRYPTO_FCN<p_rand_bytes>("RAND_bytes")(buf, n);
	}

	if (r != 1)
		throw exception("failed to generate random bytes");
}

} // namespace ssl
} // namespace snf
