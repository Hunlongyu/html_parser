#include "hps/utils/string_pool.hpp"
#include <gtest/gtest.h>

namespace hps::tests {

TEST(StringPoolTest, AddString) {
    StringPool pool;
    std::string s1 = "hello";
    auto sv1 = pool.add(s1);
    
    EXPECT_EQ(sv1, "hello");
    // Ensure sv1 points to pool memory, not s1
    EXPECT_NE(sv1.data(), s1.data());
}

TEST(StringPoolTest, AddLargeString) {
    StringPool pool(10); // Small block size
    std::string large(100, 'a');
    auto sv = pool.add(large);
    
    EXPECT_EQ(sv, large);
    EXPECT_EQ(sv.size(), 100u);
}

TEST(StringPoolTest, MultipleAllocations) {
    StringPool pool(10);
    auto sv1 = pool.add("12345");
    auto sv2 = pool.add("67890");
    auto sv3 = pool.add("abcde");
    
    EXPECT_EQ(sv1, "12345");
    EXPECT_EQ(sv2, "67890");
    EXPECT_EQ(sv3, "abcde");
}

TEST(StringPoolTest, Clear) {
    StringPool pool;
    auto sv1 = pool.add("test");
    pool.clear();
    auto sv2 = pool.add("new");
    
    EXPECT_EQ(sv2, "new");
}

} // namespace hps::tests
