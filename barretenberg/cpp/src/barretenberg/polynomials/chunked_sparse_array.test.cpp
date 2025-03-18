#include <gtest/gtest.h>
#include <stdexcept>

#include "barretenberg/polynomials/chunked_sparse_array.hpp"

TEST(ChunkedSparseArray, SetGet)
{
    ChunkedSparseArray<int> a{ 16 };

    for (size_t i = 0; i < 16; ++i) {
        EXPECT_EQ(a.get(i), 0) << "i: " << i;
    }
    EXPECT_THROW(a.get(16), std::out_of_range);

    a.set(1, 1);
    a.set(3, 3);
    a.set(9, 9);
    a.set(7, 7);
    a.set(4, 4);
    a.set(6, 6);
    a.set(8, 8);

    EXPECT_EQ(a.get(0), 0);
    EXPECT_EQ(a.get(1), 1);
    EXPECT_EQ(a.get(2), 0);
    EXPECT_EQ(a.get(3), 3);
    EXPECT_EQ(a.get(4), 4);
    EXPECT_EQ(a.get(5), 0);
    EXPECT_EQ(a.get(6), 6);
    EXPECT_EQ(a.get(7), 7);
    EXPECT_EQ(a.get(8), 8);
    EXPECT_EQ(a.get(9), 9);
    EXPECT_EQ(a.get(10), 0);
    EXPECT_EQ(a.get(11), 0);
    EXPECT_EQ(a.get(12), 0);
    EXPECT_EQ(a.get(13), 0);
    EXPECT_EQ(a.get(14), 0);
    EXPECT_EQ(a.get(15), 0);

    auto spans = a.spans();
    EXPECT_TRUE(bool(spans));
    EXPECT_EQ(spans().first, 1);
    EXPECT_TRUE(bool(spans));
    EXPECT_EQ(spans().first, 3);
    EXPECT_TRUE(bool(spans));
    EXPECT_EQ(spans().first, 6);
    EXPECT_FALSE(bool(spans));

    auto entries = a.entries();
    EXPECT_TRUE(bool(entries));
    EXPECT_EQ(entries().first, 1);
    EXPECT_EQ(entries().first, 3);
    EXPECT_EQ(entries().first, 4);
    EXPECT_EQ(entries().first, 6);
    EXPECT_EQ(entries().first, 7);
    EXPECT_EQ(entries().first, 8);
    EXPECT_EQ(entries().first, 9);
    EXPECT_FALSE(bool(entries));

    EXPECT_THROW(a.set(16, 16), std::out_of_range);
}
