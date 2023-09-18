#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <cstdint>

namespace BF
{
template <std::size_t bit_count, std::uint8_t hash_count>
class bloom_filter
{
public:
    constexpr std::size_t bit_cnt() const { return bit_count; }
    constexpr std::size_t hash_cnt() const { return hash_count; }

private:
    static constexpr std::size_t byte_count = bit_count / 8 + static_cast<bool>(bit_count & 7);

    std::uint8_t m_bits[byte_count > 0 ? byte_count : 1] = {};
};
} // BF
#endif // BLOOM_FILTER_HPP
