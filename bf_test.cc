#include "bloom_filter.hpp"
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <set>
#include <string>

namespace BF
{

inline bool is_close_enough(double a, double b, double abs_epsilon)
{
    const double delta = std::fabs(a - b);
    return delta <= abs_epsilon;
}

TEST(bf_test, ownership)
{
    {
        BF::bloom_filter bf;
        EXPECT_TRUE(bf.config(1024, 2, 3));
        EXPECT_EQ(bf.bit_count(), 1024);
        EXPECT_EQ(bf.hash_count(), 2);
        EXPECT_EQ(bf.expected_elements(), 3);
        EXPECT_GE(bf.false_positive(), 0.0);
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);

        BF::bloom_filter bf2 = std::move(bf);
        EXPECT_EQ(bf.bit_count(), 0);
        EXPECT_EQ(bf.hash_count(), 0);
        EXPECT_EQ(bf.expected_elements(), 0);
        EXPECT_EQ(bf.size(), 0);
        EXPECT_EQ(bf.false_positive(), 0.0);
        EXPECT_EQ(bf.raw(), nullptr);

        EXPECT_EQ(bf2.bit_count(), 1024);
        EXPECT_EQ(bf2.hash_count(), 2);
        EXPECT_EQ(bf2.expected_elements(), 3);
        EXPECT_GE(bf2.false_positive(), 0.0);
        EXPECT_EQ(bf2.size(), 128);
        EXPECT_NE(bf2.raw(), nullptr);

        bf = std::move(bf2);
        EXPECT_EQ(bf.bit_count(), 1024);
        EXPECT_EQ(bf.hash_count(), 2);
        EXPECT_EQ(bf.expected_elements(), 3);
        EXPECT_GE(bf.false_positive(), 0.0);
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);

        EXPECT_EQ(bf2.bit_count(), 0);
        EXPECT_EQ(bf2.hash_count(), 0);
        EXPECT_EQ(bf2.expected_elements(), 0);
        EXPECT_EQ(bf2.size(), 0);
        EXPECT_EQ(bf2.false_positive(), 0.0);
        EXPECT_EQ(bf2.raw(), nullptr);
    }
}

TEST(bf_test, parameters)
{
    {
        BF::bloom_filter bf;
        EXPECT_TRUE(bf.config(1024, 10, 1024 * 2));
        EXPECT_EQ(bf.bit_count(), 1024);
        EXPECT_EQ(bf.hash_count(), 10);
        EXPECT_EQ(bf.expected_elements(), 1024 * 2);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.999999979, 0.0000000009));
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);
    }

    {
        BF::bloom_filter bf;
        EXPECT_TRUE(bf.config(553, 0.002));
        EXPECT_EQ(bf.bit_count(), 7153);
        EXPECT_EQ(bf.hash_count(), 9);
        EXPECT_EQ(bf.expected_elements(), 553);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.002, 0.0009));
        EXPECT_EQ(bf.size(), 895);
        EXPECT_NE(bf.raw(), nullptr);
    }

    {
        BF::bloom_filter bf;
        EXPECT_FALSE(bf.config(0, 0.5));
        EXPECT_FALSE(bf.config(0, 1.0));
        EXPECT_FALSE(bf.config(0, 1.5));
        EXPECT_FALSE(bf.config(256, 0.0));
        EXPECT_FALSE(bf.config(256, -0.005));
        EXPECT_FALSE(bf.config(0, 0.0));

        EXPECT_FALSE(bf.config(0, 256, 1024));
        EXPECT_FALSE(bf.config(256, 0, 1024));
        EXPECT_FALSE(bf.config(256, 1024, 0));
        EXPECT_FALSE(bf.config(0, 0, 0));
    }
}

TEST(bf_test, from_existing_data)
{
    // assume that we received the data of an existing bf, for example over the network
    constexpr std::uint64_t byte_count = 4096;
    std::uint8_t            raw_bytes[byte_count];
    constexpr std::uint64_t n = 1001;
    constexpr std::uint64_t m = 32768;
    constexpr std::uint64_t k = 9;
    constexpr double        p = 0.000002679;

    std::srand(std::time(nullptr));
    for (std::uint64_t i = 0; i < byte_count; ++i)
        raw_bytes[i] = std::rand() % 256;

    {
        BF::bloom_filter bf;
        EXPECT_TRUE(bf.from(m, k, n, p, raw_bytes, byte_count));
        EXPECT_EQ(bf.bit_count(), m);
        EXPECT_EQ(bf.hash_count(), k);
        EXPECT_EQ(bf.expected_elements(), n);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), p, 0.0000000009));
        EXPECT_EQ(bf.size(), byte_count);
        EXPECT_NE(bf.raw(), nullptr);
        EXPECT_NE(bf.raw(), raw_bytes); // the memory location should be different
        EXPECT_EQ(std::memcmp(bf.raw(), raw_bytes, bf.size()), 0);
    }

    {
        constexpr std::uint64_t old_byte_count = 278557;
        constexpr std::uint64_t old_n          = 58123;
        constexpr std::uint64_t old_m          = 2228450;
        constexpr std::uint64_t old_k          = 27;
        constexpr double        old_p          = 0.00000001;
        BF::bloom_filter        bf;
        EXPECT_TRUE(bf.config(old_n, old_p));
        EXPECT_EQ(bf.bit_count(), old_m);
        EXPECT_EQ(bf.hash_count(), old_k);
        EXPECT_EQ(bf.expected_elements(), old_n);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), old_p, 0.000000009));
        EXPECT_EQ(bf.size(), old_byte_count);
        EXPECT_NE(bf.raw(), nullptr);
        // Override existing content
        EXPECT_TRUE(bf.from(m, k, n, p, raw_bytes, byte_count));
        EXPECT_EQ(bf.bit_count(), m);
        EXPECT_EQ(bf.hash_count(), k);
        EXPECT_EQ(bf.expected_elements(), n);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), p, 0.0000000009));
        EXPECT_EQ(bf.size(), byte_count);
        EXPECT_NE(bf.raw(), nullptr);
        EXPECT_NE(bf.raw(), raw_bytes); // the memory location should be different
        EXPECT_EQ(std::memcmp(bf.raw(), raw_bytes, bf.size()), 0);
    }
}

TEST(bf_test, hasher_sanity_check)
{
    BF::murmur3 hasher;
    {
        const std::string       input_text("");
        constexpr std::uint64_t k = 39;
        BF::hashes              out;
        out.reserve(k);
        hasher(input_text.data(), input_text.size(), k, out);
        EXPECT_EQ(out.size(), k);
    }

    {
        const std::string       input_text("This is text");
        constexpr std::uint64_t k = 1;
        BF::hashes              out, out2;
        out.reserve(k);
        out2.reserve(k);
        hasher(input_text.data(), input_text.size(), k, out);
        hasher(input_text.data(), input_text.size(), k, out2, 0x12345678);
        EXPECT_EQ(out.size(), k);
        EXPECT_EQ(out2.size(), k);
        EXPECT_NE(out[0], out2[0]);
    }

    for (std::uint64_t k = 0; k < 443; ++k)
    {
        const std::string input_text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla non ex dictum, euismod sem a, ultrices nulla.");
        BF::hashes        out;
        if (k > 1)
            out.reserve(k);

        hasher(input_text.data(), input_text.size(), k, out);
        EXPECT_EQ(out.size(), k);
        if (k > 1)
        {
            std::set<std::uint64_t> unique(out.begin(), out.end());
            EXPECT_EQ(out.size(), k);
            EXPECT_EQ(out.size(), unique.size());
        }
    }
}

TEST(bf_test, add)
{

    constexpr std::uint64_t BIT_COUNT  = 127u;
    constexpr std::uint64_t BYTE_COUNT = BIT_COUNT / 8 + static_cast<bool>(BIT_COUNT % 8);
    constexpr std::uint64_t k          = 8;
    class mock_hash
    {
    public:
        void operator()(const void* key, const std::uint64_t len, std::uint64_t k, hashes& out)
        {
            static std::uint64_t bit_index = BIT_COUNT; // bloom_filter should wrap the bit index
            static_cast<void>(key);
            static_cast<void>(len);
            static_cast<void>(k);
            for (std::uint64_t i = 0; i < k; ++i)
                out.push_back(bit_index++);
        }
    };

    BF::bloom_filter<mock_hash> bf;
    ASSERT_TRUE(bf.config(BIT_COUNT, k, 50));
    const auto raw = bf.raw();
    for (std::uint64_t i = 0; i < BYTE_COUNT; ++i)
        EXPECT_EQ(raw[i], 0x00);
    for (std::uint64_t i = 0; i < BYTE_COUNT; ++i)
        ASSERT_TRUE(bf.add(&i, sizeof(i)));
    ASSERT_TRUE(raw != nullptr);
    for (std::uint64_t i = 0; i < BYTE_COUNT; ++i)
        EXPECT_TRUE(raw[i] & 0xFF);
}

TEST(bf_test, contains)
{
    {
        BF::bloom_filter bf;
        std::string      temp("temp");
        ASSERT_FALSE(bf.contains(temp.data(), temp.size()));
        ASSERT_FALSE(bf.add(temp.data(), temp.size()));
        ASSERT_FALSE(bf.contains(temp.data(), temp.size()));
    }

    {
        BF::bloom_filter        bf;
        constexpr std::uint64_t ELEMENT_COUNT = 10000000;
        constexpr double        FPR           = 0.23;
        ASSERT_TRUE(bf.config(ELEMENT_COUNT, FPR));

        for (std::uint64_t i = 0; i < ELEMENT_COUNT; ++i)
        {
            ASSERT_TRUE(bf.add(&i, sizeof(i)));
            EXPECT_TRUE(bf.contains(&i, sizeof(i)));
        }

        std::uint64_t false_positive = 0;
        for (std::uint64_t i = ELEMENT_COUNT; i < 2 * ELEMENT_COUNT; ++i)
        {
            if (bf.contains(&i, sizeof(i)))
                false_positive++;
        }
        EXPECT_TRUE(is_close_enough(static_cast<double>(false_positive) / ELEMENT_COUNT, FPR, 0.009));
    }

    {
        BF::bloom_filter        bf;
        constexpr std::uint64_t ELEMENT_COUNT = 234;
        constexpr double        FPR           = 0.1;
        ASSERT_TRUE(bf.config(ELEMENT_COUNT, FPR));

        for (std::uint64_t i = 0; i < ELEMENT_COUNT; ++i)
        {
            ASSERT_TRUE(bf.add(&i, sizeof(i)));
            EXPECT_TRUE(bf.contains(&i, sizeof(i)));
        }

        std::uint64_t false_positive = 0;
        for (std::uint64_t i = ELEMENT_COUNT; i < 2 * ELEMENT_COUNT; ++i)
        {
            if (bf.contains(&i, sizeof(i)))
                false_positive++;
        }
        EXPECT_TRUE(is_close_enough(static_cast<double>(false_positive) / ELEMENT_COUNT, FPR, 0.09));
    }
}

TEST(bf_test, merge)
{
    constexpr std::uint64_t byte_count = 1234;
    std::uint8_t            raw_bytes[byte_count];
    constexpr std::uint64_t n = 1023;
    constexpr std::uint64_t m = byte_count * 8;
    constexpr std::uint64_t k = 2;
    constexpr double        p = 0.003322;

    for (std::uint64_t i = 0; i < byte_count; ++i)
        raw_bytes[i] = 0xAA;

    BF::bloom_filter bf;
    EXPECT_TRUE(bf.from(m, k, n, p, raw_bytes, byte_count));
    EXPECT_EQ(bf.bit_count(), m);
    EXPECT_EQ(bf.hash_count(), k);
    EXPECT_EQ(bf.expected_elements(), n);
    EXPECT_TRUE(is_close_enough(bf.false_positive(), p, 0.0000009));
    EXPECT_EQ(bf.size(), byte_count);
    EXPECT_NE(bf.raw(), nullptr);
    EXPECT_NE(bf.raw(), raw_bytes);
    EXPECT_EQ(std::memcmp(bf.raw(), raw_bytes, bf.size()), 0);

    for (std::uint64_t i = 0; i < byte_count; ++i)
        raw_bytes[i] = 0x55;

    BF::bloom_filter other;
    EXPECT_TRUE(other.from(m, k, n, p, raw_bytes, byte_count));
    EXPECT_EQ(other.bit_count(), m);
    EXPECT_EQ(other.hash_count(), k);
    EXPECT_EQ(other.expected_elements(), n);
    EXPECT_TRUE(is_close_enough(other.false_positive(), p, 0.0000009));
    EXPECT_EQ(other.size(), byte_count);
    EXPECT_NE(other.raw(), nullptr);
    EXPECT_NE(other.raw(), raw_bytes);
    EXPECT_EQ(std::memcmp(other.raw(), raw_bytes, other.size()), 0);

    EXPECT_TRUE(bf.merge(other));
    // other should not have change
    EXPECT_EQ(std::memcmp(other.raw(), raw_bytes, other.size()), 0);
    const auto bf_raw = bf.raw();
    for (std::uint64_t i = 0; i < byte_count; ++i)
        EXPECT_EQ(bf_raw[i], 0xAA | 0x55);
}

} // BF
