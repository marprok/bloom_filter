#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace bf
{
typedef std::vector<std::uint64_t> hashes;

template <typename hasher>
class bloom_filter
{
public:
    bloom_filter() noexcept
        : m(0)
        , k(0)
        , n(0)
    {
    }

    bloom_filter(const bloom_filter& other) = default;

    bloom_filter& operator=(const bloom_filter& other) = default;

    bloom_filter(bloom_filter&& other) noexcept
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

        const std::uint64_t byte_count = (m >> 3) + static_cast<bool>(m & 7);
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

        const std::uint64_t byte_count = (m >> 3) + static_cast<bool>(m & 7);
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

        const std::uint64_t byte_count = (m >> 3) + static_cast<bool>(m & 7);
        if (!raw || raw_size == 0 || byte_count != raw_size)
            return false;

        this->n = n;
        this->m = m;
        this->k = k;

        bits.clear();
        bits.resize(raw_size);
        std::memcpy(bits.data(), raw, raw_size);

        return true;
    }

    inline std::uint64_t bit_count() const { return m; }

    inline std::uint64_t hash_count() const { return k; }

    inline std::uint64_t expected_elements() const { return n; }

    inline double false_positive() const { return compute_p(m, k, n); }

    inline std::size_t size() const { return bits.size(); } // in bytes

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

        const std::uint64_t seed = h(key, len);
        if (k == 1) [[unlikely]]
        {
            const std::uint64_t abs_bit_id = seed % m;
            const std::uint64_t byte_id    = abs_bit_id >> 3;
            bits[byte_id] |= (1U << (abs_bit_id & 7));
        }
        else [[likely]]
        {
            const std::uint64_t h0 = (seed & 0xFFFFFFFF00000000) >> 32;
            const std::uint64_t h1 = seed & 0xFFFFFFFF;
            for (std::uint64_t i = 0; i < k; ++i)
            {
                const std::uint64_t abs_bit_id = (h0 * (i != 1) + i * h1) % m;
                const std::uint64_t byte_id    = abs_bit_id >> 3;
                bits[byte_id] |= (1U << (abs_bit_id & 7));
            }
        }
        return true;
    }

    bool contains(const void* key, const std::uint64_t len) const
    {
        if (m == 0 || k == 0 || n == 0)
            return false;

        const std::uint64_t seed = h(key, len);
        if (k == 1) [[unlikely]]
        {
            const std::uint64_t abs_bit_id = seed % m;
            const std::uint64_t byte_id    = abs_bit_id >> 3;
            if (!(bits[byte_id] & (1U << (abs_bit_id & 7))))
                return false;
        }
        else [[likely]]
        {
            const std::uint64_t h0 = (seed & 0xFFFFFFFF00000000) >> 32;
            const std::uint64_t h1 = seed & 0xFFFFFFFF;
            for (std::uint64_t i = 0; i < k; ++i)
            {
                const std::uint64_t abs_bit_id = (h0 * (i != 1) + i * h1) % m;
                const std::uint64_t byte_id    = abs_bit_id >> 3;
                if (!(bits[byte_id] & (1U << (abs_bit_id & 7))))
                    return false;
            }
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

    inline void clear()
    {
        m = k = n = 0;
        bits.clear();
    }

    bool operator==(const bloom_filter& other) const
    {
        if (m != other.m || k != other.k || n != other.n)
            return false;

        return bits == other.bits;
    }

private:
    std::uint64_t m; // size in bits
    std::uint64_t k; // number of hashes
    std::uint64_t n; // expected number of elements
    hasher        h;
    // TODO: maybe fetch 64 bits at a time?
    std::vector<std::uint8_t> bits;

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

} // bf
#endif // BLOOM_FILTER_HPP
