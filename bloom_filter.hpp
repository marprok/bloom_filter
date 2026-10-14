#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace BF
{
typedef std::vector<std::uint64_t> hashes;

template <typename hasher>
class bloom_filter
{
public:
    bloom_filter()
        : m(0)
        , k(0)
        , n(0)
    {
    }

    bloom_filter(const bloom_filter& other) = default;

    bloom_filter& operator=(const bloom_filter& other) = default;

    bloom_filter(bloom_filter&& other)
        : m(other.m)
        , k(other.k)
        , n(other.n)
    {
        if (!other.bits.empty())
            bits = std::move(other.bits);

        other.clear();
    }

    bloom_filter& operator=(bloom_filter&& other)
    {
        if (this != &other)
        {
            m = other.m;
            k = other.k;
            n = other.n;

            if (!other.bits.empty())
                bits = std::move(other.bits);

            other.clear();
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
        bits.clear();
        bits.resize(byte_count > 0 ? byte_count : 1, 0);

        return true;
    }

    bool config(std::uint64_t n, double p)
    {
        if (p >= 1.0 || p <= 0.0 || n == 0)
            return false;

        this->n = n;
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
              const std::uint8_t* raw,
              std::uint64_t       raw_size)
    {
        const double p = compute_p(m, k, n);
        if (p >= 1.0 || p <= 0.0 || n == 0)
            return false;

        const std::uint64_t byte_count = m / 8 + static_cast<bool>(m & 7);
        if (!raw || raw_size == 0 || byte_count != raw_size)
            return false;

        this->n = n;
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

    double false_positive() const { return compute_p(m, k, n); }

    std::size_t size() const { return bits.size(); } // in bytes

    const std::uint8_t* raw() const
    {
        if (bits.empty())
            return nullptr;
        return bits.data();
    }

    bool add(const void* key, const std::uint64_t len)
    {
        if (m == 0 || k == 0 || n == 0)
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
        if (m == 0 || k == 0 || n == 0)
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

    bool merge(const bloom_filter& other)
    {
        if (m == 0 || k == 0 || n == 0)
            return false;

        if (m != other.m || k != other.k || n != other.n
            || bits.size() != other.bits.size())
            return false;

        for (std::uint64_t i = 0; i < bits.size(); ++i)
            bits[i] |= other.bits[i];

        return true;
    }

    void clear()
    {
        m = k = n = 0;
        bits.clear();
    }

    constexpr bool operator==(const bloom_filter& other) const
    {
        if (m != other.m || k != other.k || n != other.n)
            return false;

        return bits == other.bits;
    }

private:
    static constexpr std::uint8_t BIT_POS[8] = { 0x1u, 0x2u, 0x4u, 0x8u, 0x10u, 0x20u, 0x40u, 0x80u };

    std::uint64_t             m; // size in bits
    std::uint64_t             k; // number of hashes
    std::uint64_t             n; // expected number of elements
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

    inline double compute_p(std::uint64_t m, std::uint64_t k, std::uint64_t n) const
    {
        return std::pow(1.0 - std::exp((-static_cast<double>(k) * n) / m), k);
    }
};

} // BF
#endif // BLOOM_FILTER_HPP
