#include "bloom_filter.hpp"
#include <gtest/gtest.h>

namespace BF
{
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
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.999999979));
        EXPECT_EQ(bf.size(), 128);
        EXPECT_NE(bf.raw(), nullptr);
    }

    {
        BF::bloom_filter bf;
        EXPECT_TRUE(bf.config(553, 0.002));
        EXPECT_EQ(bf.bit_count(), 7153);
        EXPECT_EQ(bf.hash_count(), 9);
        EXPECT_EQ(bf.expected_elements(), 553);
        EXPECT_TRUE(is_close_enough(bf.false_positive(), 0.002));
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
} // BF
