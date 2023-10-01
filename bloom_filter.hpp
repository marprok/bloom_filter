#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace BF
{
void MurmurHash3_x64_128_marios(const void* key, const int len, const uint32_t seed, void* out)
{

    constexpr auto ROTL64 = [](uint64_t x, int8_t r) constexpr->uint64_t
    {
        return (x << r) | (x >> (64 - r));
    };

    constexpr auto fmix64 = [](uint64_t k) constexpr->uint64_t
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdLLU;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53LLU;
        k ^= k >> 33;

        return k;
    };

    constexpr auto getblock64 = [](const uint64_t* p, int i) constexpr->uint64_t
    {
        return p[i];
    };

    const uint8_t* data    = (const uint8_t*)key;
    const int      nblocks = len / 16;

    uint64_t h1 = seed;
    uint64_t h2 = seed;

    const uint64_t c1 = 0x87c37b91114253d5LLU;
    const uint64_t c2 = 0x4cf5ad432745937fLLU;

    //----------
    // body

    const uint64_t* blocks = (const uint64_t*)(data);

    for (int i = 0; i < nblocks; i++)
    {
        uint64_t k1 = getblock64(blocks, i * 2 + 0);
        uint64_t k2 = getblock64(blocks, i * 2 + 1);

        k1 *= c1;
        k1 = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1 = ROTL64(h1, 27);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2 = ROTL64(h2, 31);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    //----------
    // tail

    const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    switch (len & 15)
    {
    case 15:
        k2 ^= ((uint64_t)tail[14]) << 48;
    case 14:
        k2 ^= ((uint64_t)tail[13]) << 40;
    case 13:
        k2 ^= ((uint64_t)tail[12]) << 32;
    case 12:
        k2 ^= ((uint64_t)tail[11]) << 24;
    case 11:
        k2 ^= ((uint64_t)tail[10]) << 16;
    case 10:
        k2 ^= ((uint64_t)tail[9]) << 8;
    case 9:
        k2 ^= ((uint64_t)tail[8]) << 0;
        k2 *= c2;
        k2 = ROTL64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

    case 8:
        k1 ^= ((uint64_t)tail[7]) << 56;
    case 7:
        k1 ^= ((uint64_t)tail[6]) << 48;
    case 6:
        k1 ^= ((uint64_t)tail[5]) << 40;
    case 5:
        k1 ^= ((uint64_t)tail[4]) << 32;
    case 4:
        k1 ^= ((uint64_t)tail[3]) << 24;
    case 3:
        k1 ^= ((uint64_t)tail[2]) << 16;
    case 2:
        k1 ^= ((uint64_t)tail[1]) << 8;
    case 1:
        k1 ^= ((uint64_t)tail[0]) << 0;
        k1 *= c1;
        k1 = ROTL64(k1, 31);
        k1 *= c2;
        h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    ((uint64_t*)out)[0] = h1;
    ((uint64_t*)out)[1] = h2;
}

inline bool is_close_enough(double a, double b)
{
    constexpr double EPSILON = 0.000000001;
    const double     delta   = std::fabs(a - b);
    return delta <= EPSILON;
}

class bloom_filter
{
public:
    // None of the constructors take care of div by zero. They will just crash.
    bloom_filter()
        : m(0)
        , k(0)
        , n(0)
        , p(0.0)
    {
    }

    bloom_filter(const bloom_filter& other) = default;

    bloom_filter& operator=(const bloom_filter& other) = default;

    bloom_filter(bloom_filter&& other)
        : m(other.m)
        , k(other.k)
        , n(other.n)
        , p(other.p)
    {
        if (!other.bits.empty())
            std::swap(bits, other.bits);
        other.m = other.k = other.n = other.p = 0;
    }

    bloom_filter& operator=(bloom_filter&& other)
    {
        if (this != &other)
        {
            m = other.m;
            k = other.k;
            n = other.n;
            p = other.p;

            if (!other.bits.empty())
            {
                std::swap(bits, other.bits);
                other.bits.clear(); // in case it is not empty
            }
            other.m = other.k = other.n = other.p = 0;
        }
        return *this;
    }

    bool config(std::uint64_t m, std::uint64_t k, std::uint64_t n)
    {
        if (m == 0 || k == 0 || n == 0)
            return false;

        this->m = m;
        this->k = k;
        this->n = n;

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        bits.resize(byte_count > 0 ? byte_count : 1, 0);
        p = std::pow(1.0 - std::exp((-static_cast<double>(k) * n) / m), k);

        return true;
    }

    bool config(std::uint64_t n, double p)
    {
        if (is_close_enough(p, 0.0)
            || p < 0.0
            || is_close_enough(p, 1.0)
            || p > 1.0
            || n == 0)
            return false;

        this->n = n;
        this->p = p;

        m = std::ceil((n * std::log(p)) / std::log(1.0 / std::pow(2.0, std::log(2.0))));
        k = std::round((static_cast<double>(m) / n) * std::log(2.0));

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        bits.resize(byte_count > 0 ? byte_count : 1, 0);

        return true;
    }

    // Does not check for the values of p and the size of the buffer
    // correspond to the m, k, n parameters. TODO: fix this
    bool from(std::uint64_t       m,
              std::uint64_t       k,
              std::uint64_t       n,
              double              p,
              const std::uint8_t* raw,
              std::uint64_t       raw_size)
    {
        if (!raw)
            return false;

        this->m = m;
        this->k = k;
        this->n = n;
        this->p = p;
        bits.reserve(raw_size);
        std::copy(raw, raw + raw_size, bits.begin());

        return true;
    }

    std::uint64_t bit_count() const { return m; }
    std::uint64_t hash_count() const { return k; }
    std::uint64_t expected_elements() const { return n; }
    std::size_t   size() const { return bits.size(); } // in bytes
    double        false_positive() const { return p; }

    const std::uint8_t* raw() const
    {
        if (bits.empty())
            return nullptr;

        return bits.data();
    }

private:
    std::uint64_t m; // size in bits
    std::uint64_t k; // number of hashes
    std::uint64_t n; // expected number of elements
    double        p; // false positive probability(> 0 && < 1) TODO: maybe long double?

    std::vector<std::uint8_t> bits;
};
} // BF
#endif // BLOOM_FILTER_HPP
