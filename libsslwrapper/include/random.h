#ifndef _SNF_RANDOM_H_
#define _SNF_RANDOM_H_

#include <random>
#include <limits>
#include "common.h"
#include "sslfcn.h"
#include "safestr.h"

namespace snf {
namespace ssl {

class random
{
private:
	std::random_device              m_random;
	std::default_random_engine      m_engine;

public:
	random();

	template<typename I> I integral(I min = 0, I max = std::numeric_limits<I>::max(), EnableIfIntegral<I> * = 0)
	{
		std::uniform_int_distribution<I> d(min, max);
		return d(m_engine);
	}

	template<typename R> R real(R min = 0.0, R max = std::numeric_limits<R>::max(), EnableIfReal<R> * = 0)
	{
		std::uniform_real_distribution<R> d(min, max);
		return d(m_engine);
	}

	void bytes(safestr &, bool private_prng_instance = false);
	void bytes(uint8_t *, size_t, bool private_prng_instance = false);
};

} // namespace ssl
} // namespace snf

#endif // _SNF_RANDOM_H_
