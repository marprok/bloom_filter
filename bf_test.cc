#include "bloom_filter.hpp"
#include <gtest/gtest.h>

namespace BF
{
TEST(bf_test, size_test)
{
    {
        BF::bloom_filter<32, 5> bf;
        EXPECT_EQ(bf.bit_cnt(), 32);
        EXPECT_EQ(bf.hash_cnt(), 5);
        EXPECT_EQ(sizeof(bf), 4);
    }

    {
        BF::bloom_filter<0, 65> bf;
        EXPECT_EQ(bf.bit_cnt(), 0);
        EXPECT_EQ(bf.hash_cnt(), 65);
        EXPECT_EQ(sizeof(bf), 1);
    }

    {
        BF::bloom_filter<12, 255> bf;
        EXPECT_EQ(bf.bit_cnt(), 12);
        EXPECT_EQ(bf.hash_cnt(), 255);
        EXPECT_EQ(sizeof(bf), 2);
    }

    {
        BF::bloom_filter<674, 0> bf;
        EXPECT_EQ(bf.bit_cnt(), 674);
        EXPECT_EQ(bf.hash_cnt(), 0);
        EXPECT_EQ(sizeof(bf), 85);
    }
}
} // BF
