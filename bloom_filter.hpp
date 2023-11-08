#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace BF
{
typedef std::vector<std::uint64_t> hashes;

class murmur3
{
public:
    // taken from: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
    void operator()(const void* key, const std::uint64_t len, std::uint64_t k, hashes& out, const std::uint32_t seed = 0xbeefeebb) const
    {
        // do not do any work if it is not needed...
        if (k == 0)
            return;

        const std::uint8_t*  data    = (const std::uint8_t*)key;
        const std::uint64_t  nblocks = len / 16;
        std::uint64_t        h1      = seed;
        std::uint64_t        h2      = seed;
        const std::uint64_t  c1      = 0x87c37b91114253d5LLU;
        const std::uint64_t  c2      = 0x4cf5ad432745937fLLU;
        const std::uint64_t* blocks  = (const std::uint64_t*)(data);

        for (std::uint64_t i = 0; i < nblocks; i++)
        {
            std::uint64_t k1 = getblock64(blocks, i * 2 + 0);
            std::uint64_t k2 = getblock64(blocks, i * 2 + 1);

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

        const std::uint8_t* tail = (const std::uint8_t*)(data + nblocks * 16);
        std::uint64_t       k1   = 0;
        std::uint64_t       k2   = 0;

        switch (len & 15)
        {
        case 15:
            k2 ^= ((std::uint64_t)tail[14]) << 48;
        case 14:
            k2 ^= ((std::uint64_t)tail[13]) << 40;
        case 13:
            k2 ^= ((std::uint64_t)tail[12]) << 32;
        case 12:
            k2 ^= ((std::uint64_t)tail[11]) << 24;
        case 11:
            k2 ^= ((std::uint64_t)tail[10]) << 16;
        case 10:
            k2 ^= ((std::uint64_t)tail[9]) << 8;
        case 9:
            k2 ^= ((std::uint64_t)tail[8]) << 0;
            k2 *= c2;
            k2 = ROTL64(k2, 33);
            k2 *= c1;
            h2 ^= k2;
        case 8:
            k1 ^= ((std::uint64_t)tail[7]) << 56;
        case 7:
            k1 ^= ((std::uint64_t)tail[6]) << 48;
        case 6:
            k1 ^= ((std::uint64_t)tail[5]) << 40;
        case 5:
            k1 ^= ((std::uint64_t)tail[4]) << 32;
        case 4:
            k1 ^= ((std::uint64_t)tail[3]) << 24;
        case 3:
            k1 ^= ((std::uint64_t)tail[2]) << 16;
        case 2:
            k1 ^= ((std::uint64_t)tail[1]) << 8;
        case 1:
            k1 ^= ((std::uint64_t)tail[0]) << 0;
            k1 *= c1;
            k1 = ROTL64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
        };

        h1 ^= len;
        h2 ^= len;
        h1 += h2;
        h2 += h1;
        h1 = fmix64(h1);
        h2 = fmix64(h2);
        h1 += h2;
        h2 += h1;

        if (k == 1)
            out.push_back(h1);
        else if (k > 1)
        {
            out.push_back(h1);
            out.push_back(h2);
            // apply the Kirsch-Mitzenmacher-Optimization
            for (std::uint64_t i = 3; i <= k; ++i)
            {
                auto g = h1 + i * h2;
                out.push_back(g);
                std::swap(h1, h2);
                std::swap(h2, g);
            }
        }
    }

private:
    inline std::uint64_t ROTL64(std::uint64_t x, std::int8_t r) const
    {
        return (x << r) | (x >> (64 - r));
    }

    inline std::uint64_t fmix64(std::uint64_t k) const
    {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdLLU;
        k ^= k >> 33;
        k *= 0xc4ceb9fe1a85ec53LLU;
        k ^= k >> 33;

        return k;
    }

    inline std::uint64_t getblock64(const std::uint64_t* p, std::uint64_t i) const
    {
        return p[i];
    }
};

template <typename hasher = murmur3>
class bloom_filter
{
public:
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
        this->p = compute_p(m, k, n);

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        bits.clear();
        bits.resize(byte_count > 0 ? byte_count : 1, 0);

        return true;
    }

    bool config(std::uint64_t n, double p)
    {
        if (p >= 1.0 || p <= 0.0 || n == 0)
            return false;

        this->n = n;
        this->p = p;
        m       = compute_m(n, p);
        k       = compute_k(m, n);

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        bits.clear();
        bits.resize(byte_count > 0 ? byte_count : 1, 0);

        return true;
    }

    // Create a bf from the components of an existing one.
    // Deep copies the values from the raw byte pointer.
    bool from(std::uint64_t       m,
              std::uint64_t       k,
              std::uint64_t       n,
              double              p,
              const std::uint8_t* raw,
              std::uint64_t       raw_size)
    {
        if (p >= 1.0 || p <= 0.0 || n == 0)
            return false;

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        if (!raw || raw_size == 0 || byte_count != raw_size)
            return false;

        this->n = n;
        this->p = p;
        this->m = m;
        this->k = k;

        bits.clear();
        bits.reserve(raw_size);
        std::copy(raw, raw + raw_size, std::back_inserter(bits));

        return true;
    }

    std::uint64_t bit_count() const { return m; }

    std::uint64_t hash_count() const { return k; }

    std::uint64_t expected_elements() const { return n; }

    double false_positive() const { return p; }

    std::size_t size() const { return bits.size(); } // in bytes

    const std::uint8_t* raw() const
    {
        if (bits.empty())
            return nullptr;
        return bits.data();
    }

    bool add(const void* key, const std::uint64_t len)
    {
        if (m == 0 || k == 0 || n == 0 || p == 0.0)
            return false;

        hashes hash_values;
        hash_values.reserve(k);
        h(key, len, k, hash_values);

        if (k != hash_values.size())
            return false;

        for (std::uint64_t i = 0; i < k; ++i)
        {
            const std::uint64_t abs_bit_id = hash_values[i] % m;
            const std::uint64_t byte_id    = abs_bit_id / 8;
            bits[byte_id] |= BIT_POS[abs_bit_id & 7];
        }
        return true;
    }

    bool contains(const void* key, const std::uint64_t len) const
    {
        if (m == 0 || k == 0 || n == 0 || p == 0.0)
            return false;

        hashes hash_values;
        hash_values.reserve(k);
        h(key, len, k, hash_values);

        if (k != hash_values.size())
            return false;

        for (std::uint64_t i = 0; i < k; ++i)
        {
            const std::uint64_t abs_bit_id = hash_values[i] % m;
            const std::uint64_t byte_id    = abs_bit_id / 8;
            if (!(bits[byte_id] & BIT_POS[abs_bit_id & 7]))
                return false;
        }
        return true;
    }

private:
    static constexpr std::uint8_t BIT_POS[8] = { 0x1u, 0x2u, 0x4u, 0x8u, 0x10u, 0x20u, 0x40u, 0x80u };

    std::uint64_t             m; // size in bits
    std::uint64_t             k; // number of hashes
    std::uint64_t             n; // expected number of elements
    double                    p; // false positive probability(> 0 && < 1)
    std::vector<std::uint8_t> bits;
    hasher                    h;

    inline std::uint64_t compute_m(std::uint64_t n, double p) const
    {
        return std::ceil((n * std::log(p)) / std::log(1.0 / std::pow(2.0, std::log(2.0))));
    }

    inline std::uint64_t compute_k(std::uint64_t m, std::uint64_t n) const
    {
        return std::round((static_cast<double>(m) / n) * std::log(2.0));
    }

    inline double compute_p(std::uint64_t m, std::uint64_t k, std::uint64_t n)
    {
        return std::pow(1.0 - std::exp((-static_cast<double>(k) * n) / m), k);
    }
};

} // BF
#endif // BLOOM_FILTER_HPP
