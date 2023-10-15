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
        BF::murmur3::hashes     out;
        out.reserve(k);

        hasher(input_text.data(), input_text.size(), k, out);
        EXPECT_EQ(out.size(), k);
    }

    {
        const std::string       input_text("This is text");
        constexpr std::uint64_t k = 1;
        BF::murmur3::hashes     out, out2;
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
        const std::string   input_text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla non ex dictum, euismod sem a, ultrices nulla.");
        BF::murmur3::hashes out;
        if (k > 1)
            out.reserve(k);

        hasher(input_text.data(), input_text.size(), k, out);
        EXPECT_EQ(out.size(), k);
        if (k > 1)
        {
            std::set<std::uint64_t> unique;
            for (const auto h : out)
                unique.insert(h);

            EXPECT_EQ(out.size(), k);
            EXPECT_EQ(out.size(), unique.size());
        }
    }
}

} // BF
