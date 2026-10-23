#include "bloom_filter.hpp"
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <string>

namespace BF
{

inline bool is_close_enough(double a, double b, double abs_epsilon)
{
    const double delta = std::fabs(a - b);
    return delta <= abs_epsilon;
}

class hash_string
{
public:
    inline std::uint64_t operator()(const void* key, const std::uint64_t len) const
    {
        return std::hash<std::string> {}(std::string(reinterpret_cast<const char*>(key), len));
    }
};

TEST(bf_test, ownership)
{
    {
        BF::bloom_filter<hash_string> bf;
        EXPECT_TRUE(bf.config(1024, 2, 3));
        EXPECT_EQ(bf.bit_count(), 1024);
        EXPECT_EQ(bf.hash_count(), 2);
        EXPECT_EQ(bf.expected_elements(), 3);
        EXPECT_GE(bf.false_positive(), 0.0);
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);

        BF::bloom_filter<hash_string> bf2 = std::move(bf);
        EXPECT_EQ(bf.bit_count(), 0);
        EXPECT_EQ(bf.hash_count(), 0);
        EXPECT_EQ(bf.expected_elements(), 0);
        EXPECT_EQ(bf.size(), 0);
        EXPECT_EQ(bf.raw(), nullptr);

        EXPECT_EQ(bf.false_positive(), 1.0);
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
        EXPECT_EQ(bf2.false_positive(), 1.0);
        EXPECT_EQ(bf2.raw(), nullptr);

        BF::bloom_filter<hash_string> bf3 = bf;
        EXPECT_EQ(bf3, bf);

        bf.clear();
        EXPECT_EQ(bf.bit_count(), 0);
        EXPECT_EQ(bf.hash_count(), 0);
        EXPECT_EQ(bf.expected_elements(), 0);
        EXPECT_EQ(bf.size(), 0);
        EXPECT_EQ(bf.false_positive(), 1.0);
        EXPECT_EQ(bf.raw(), nullptr);
    }
}

TEST(bf_test, parameters)
{
    {
        BF::bloom_filter<hash_string> bf;
        EXPECT_TRUE(bf.config(1024, 10, 1024 * 2));
        EXPECT_EQ(bf.bit_count(), 1024);
        EXPECT_EQ(bf.hash_count(), 10);
        EXPECT_EQ(bf.expected_elements(), 1024 * 2);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.999999979, 0.0000000009));
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);
    }

    {
        BF::bloom_filter<hash_string> bf;
        EXPECT_TRUE(bf.config(553, 0.002));
        EXPECT_EQ(bf.bit_count(), 7153);
        EXPECT_EQ(bf.hash_count(), 9);
        EXPECT_EQ(bf.expected_elements(), 553);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.002, 0.00009));
        EXPECT_EQ(bf.size(), 895);
        EXPECT_NE(bf.raw(), nullptr);
    }

    {
        BF::bloom_filter<hash_string> bf;
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
        BF::bloom_filter<hash_string> bf;
        EXPECT_TRUE(bf.from(m, k, n, raw_bytes, byte_count));
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
        constexpr std::uint64_t       old_byte_count = 278557;
        constexpr std::uint64_t       old_n          = 58123;
        constexpr std::uint64_t       old_m          = 2228450;
        constexpr std::uint64_t       old_k          = 27;
        constexpr double              old_p          = 0.00000001;
        BF::bloom_filter<hash_string> bf;
        EXPECT_TRUE(bf.config(old_n, old_p));
        EXPECT_EQ(bf.bit_count(), old_m);
        EXPECT_EQ(bf.hash_count(), old_k);
        EXPECT_EQ(bf.expected_elements(), old_n);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), old_p, 0.000000009));
        EXPECT_EQ(bf.size(), old_byte_count);
        EXPECT_NE(bf.raw(), nullptr);
        // Override existing content
        EXPECT_TRUE(bf.from(m, k, n, raw_bytes, byte_count));
        EXPECT_EQ(bf.bit_count(), m);
        EXPECT_EQ(bf.hash_count(), k);
        EXPECT_EQ(bf.expected_elements(), n);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), p, 0.000000009));
        EXPECT_EQ(bf.size(), byte_count);
        EXPECT_NE(bf.raw(), nullptr);
        EXPECT_NE(bf.raw(), raw_bytes); // the memory location should be different
        EXPECT_EQ(std::memcmp(bf.raw(), raw_bytes, bf.size()), 0);
    }
}

TEST(bf_test, add)
{

    constexpr std::uint64_t BIT_COUNT  = 127u;
    constexpr std::uint64_t BYTE_COUNT = (BIT_COUNT >> 3) + static_cast<bool>(BIT_COUNT % 8);
    constexpr std::uint64_t k          = 8;
    class mock_hash
    {
    public:
        inline std::uint64_t operator()(const void* key, const std::uint64_t len) const
        {
            static_cast<void>(len);

            return (*reinterpret_cast<const std::uint64_t*>(key)) * 8;
        }
    };

    BF::bloom_filter<mock_hash> bf;
    ASSERT_TRUE(bf.config(BIT_COUNT, k, BYTE_COUNT));
    const std::uint8_t* raw = bf.raw();
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
        BF::bloom_filter<hash_string> bf;
        constexpr std::string_view    temp("temp");
        ASSERT_FALSE(bf.contains(temp.data(), temp.size()));
        ASSERT_FALSE(bf.add(temp.data(), temp.size()));
        ASSERT_FALSE(bf.contains(temp.data(), temp.size()));
    }

    {
        BF::bloom_filter<hash_string> bf;
        constexpr std::uint64_t       ELEMENT_COUNT = 10000000;
        constexpr double              FPR           = 0.23;
        ASSERT_TRUE(bf.config(ELEMENT_COUNT, FPR));

        for (std::uint64_t i = 0; i < ELEMENT_COUNT; ++i)
        {
            const std::string s = std::to_string(i);
            ASSERT_TRUE(bf.add(s.data(), s.size()));
            EXPECT_TRUE(bf.contains(s.data(), s.size()));
        }

        std::uint64_t false_positive = 0;
        for (std::uint64_t i = ELEMENT_COUNT; i < 2 * ELEMENT_COUNT; ++i)
        {
            const std::string s = std::to_string(i);
            if (bf.contains(s.data(), s.size()))
                false_positive++;
        }
        EXPECT_TRUE(is_close_enough(static_cast<double>(false_positive) / ELEMENT_COUNT, FPR, 0.009));
    }

    {
        BF::bloom_filter<hash_string> bf;
        constexpr std::uint64_t       ELEMENT_COUNT = 234;
        constexpr double              FPR           = 0.1;
        ASSERT_TRUE(bf.config(ELEMENT_COUNT, FPR));

        for (std::uint64_t i = 0; i < ELEMENT_COUNT; ++i)
        {
            const std::string s = std::to_string(i);
            ASSERT_TRUE(bf.add(s.data(), s.size()));
            EXPECT_TRUE(bf.contains(s.data(), s.size()));
        }

        std::uint64_t false_positive = 0;
        for (std::uint64_t i = ELEMENT_COUNT; i < 2 * ELEMENT_COUNT; ++i)
        {
            const std::string s = std::to_string(i);
            if (bf.contains(s.data(), s.size()))
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
    constexpr double        p = 0.035038556;

    for (std::uint64_t i = 0; i < byte_count; ++i)
        raw_bytes[i] = 0xAA;

    BF::bloom_filter<hash_string> bf;
    EXPECT_TRUE(bf.from(m, k, n, raw_bytes, byte_count));
    EXPECT_EQ(bf.bit_count(), m);
    EXPECT_EQ(bf.hash_count(), k);
    EXPECT_EQ(bf.expected_elements(), n);
    EXPECT_TRUE(is_close_enough(bf.false_positive(), p, 0.0000000009));
    EXPECT_EQ(bf.size(), byte_count);
    EXPECT_NE(bf.raw(), nullptr);
    EXPECT_NE(bf.raw(), raw_bytes);
    EXPECT_EQ(std::memcmp(bf.raw(), raw_bytes, bf.size()), 0);

    for (std::uint64_t i = 0; i < byte_count; ++i)
        raw_bytes[i] = 0x55;

    BF::bloom_filter<hash_string> other;
    EXPECT_TRUE(other.from(m, k, n, raw_bytes, byte_count));
    EXPECT_EQ(other.bit_count(), m);
    EXPECT_EQ(other.hash_count(), k);
    EXPECT_EQ(other.expected_elements(), n);
    EXPECT_TRUE(is_close_enough(other.false_positive(), p, 0.0000000009));
    EXPECT_EQ(other.size(), byte_count);
    EXPECT_NE(other.raw(), nullptr);
    EXPECT_NE(other.raw(), raw_bytes);
    EXPECT_EQ(std::memcmp(other.raw(), raw_bytes, other.size()), 0);

    EXPECT_TRUE(bf.merge(other));
    // other should not have change
    EXPECT_EQ(std::memcmp(other.raw(), raw_bytes, other.size()), 0);
    const std::uint8_t* bf_raw = bf.raw();
    for (std::uint64_t i = 0; i < byte_count; ++i)
        EXPECT_EQ(bf_raw[i], 0xAA | 0x55);
}

TEST(bf_test, equality)
{
    BF::bloom_filter<hash_string> first;
    BF::bloom_filter<hash_string> second;
    EXPECT_EQ(first, second);

    constexpr std::uint64_t n = 512;
    constexpr std::uint64_t m = 2048;
    constexpr std::uint64_t k = 4;

    EXPECT_TRUE(first.config(m, k, n));
    EXPECT_TRUE(second.config(m, k, n));

    EXPECT_EQ(first, second);

    constexpr std::string_view v1("this is the first test");
    constexpr std::string_view v2("this is the second test");
    constexpr std::string_view v3("this is the third test");

    first.add(v1.data(), v1.size());
    second.add(v1.data(), v1.size());
    EXPECT_EQ(first, second);

    first.add(v2.data(), v2.size());
    second.add(v2.data(), v2.size());
    EXPECT_EQ(first, second);

    first.add(v3.data(), v3.size());
    EXPECT_NE(first, second);
    second.add(v3.data(), v3.size());
    EXPECT_EQ(first, second);

    BF::bloom_filter<hash_string> third;
    EXPECT_TRUE(third.config(m - 1, k - 1, n - 1));
    third.add(v1.data(), v1.size());
    third.add(v2.data(), v2.size());
    third.add(v3.data(), v3.size());

    EXPECT_NE(first, third);
    EXPECT_NE(second, third);
    third = std::move(first); // first is no longer valid
    EXPECT_EQ(second, third);
    EXPECT_NE(first, third);

    BF::bloom_filter<hash_string> fourth(std::move(second)); // second is no longer valid
    EXPECT_EQ(third, fourth);
    EXPECT_NE(second, third);

    BF::bloom_filter<hash_string> fifth = fourth;
    EXPECT_EQ(fifth, fourth);
    EXPECT_EQ(fifth, third);
}

} // BF
