#include <algorithm>
#include <array>
#include <cstdint>
#include <random>

namespace martinus {
namespace random {
namespace detail {

// Mixin class to extend plain random number generator engines
template <typename Engine> class Extras {
public:
	double real01()
	{
		static_assert(sizeof(typename Engine::result_type) == 8);

		union {
			uint64_t i;
			double d;
		} x;
		x.i = (UINT64_C(0x3ff) << 52) | (engine()() >> 12);
		return x.d - 1.0;
	}

	double real_between(double min_val, double max_val) { return (max_val - min_val) * real01(); }

private:
	// Since this is the CRTP, it's safe to do a static cast.
	Engine& engine() { return *static_cast<Engine*>(this); }

	// Ensure correctly correct derivation of the CRTP:
	// make all ctor's private, but only Engine a friend
	Extras() {}
	friend Engine;
};

template <typename T> T rotl(T const x, int k) { return (x << k) | (x >> (8 * sizeof(T) - k)); }

// random number based on std::random_device. Probably slow. Use to seed your RNG.
uint64_t rand64()
{
	std::random_device device;
	std::uniform_int_distribution<uint64_t> dis;
	return dis(device);
};

} // namespace detail

class splitmix64 : public detail::Extras<splitmix64> {
public:
	using result_type = uint64_t;
	using state_type = uint64_t;

	splitmix64(uint64_t seed)
		: m_state(seed)
	{
	}

	splitmix64()
		: m_state(detail::rand64())
	{
	}

	uint64_t operator()()
	{
		uint64_t z = (m_state += UINT64_C(0x9E3779B97F4A7C15));
		z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
		z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
		return z ^ (z >> 31);
	}

private:
	state_type m_state;
};

// based on http://xoshiro.di.unimi.it/xoshiro256starstar.c
class xoshiro256starstar : public detail::Extras<xoshiro256starstar> {
public:
	using result_type = uint64_t;
	using state_type = std::array<uint64_t, 4>;

	xoshiro256starstar(uint64_t seed)
	{
		std::generate(std::begin(m_state), std::end(m_state), splitmix64{ seed });
	}

	xoshiro256starstar()
		: xoshiro256starstar(detail::rand64())
	{
	}

	uint64_t operator()()
	{
		uint64_t const result_starstar = detail::rotl(m_state[1] * 5, 7) * 9;

		uint64_t const t = m_state[1] << 17;

		m_state[2] ^= m_state[0];
		m_state[3] ^= m_state[1];
		m_state[1] ^= m_state[2];
		m_state[0] ^= m_state[3];

		m_state[2] ^= t;

		m_state[3] = detail::rotl(m_state[3], 45);

		return result_starstar;
	}

private:
	state_type m_state;
};

} // namespace random
} // namespace martinus

#include <iostream>

int main(int, char**)
{
	using namespace martinus::random;
	xoshiro256starstar rng(123);
	for (size_t i = 0; i < 10; ++i) {
		std::cout << rng.real01() << std::endl;
	}
}
