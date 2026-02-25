#include <cstdint>
#include <cstring>
#include <limits>

#include "gtest/gtest.h"
#include "sketch/add.h"

namespace sketch {

namespace {

TEST(Add, Simple) {
  int32_t a = 2;
  int32_t b = 3;

  ASSERT_EQ(Add(a, b), 5);
}

TEST(Add, WithOverflow) {
  int32_t a = std::numeric_limits<int32_t>::max();
  int32_t b = 1;

  int64_t expected = static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1;

  ASSERT_EQ(Add(a, b), expected);
}

}  // namespace

}  // namespace sketch
