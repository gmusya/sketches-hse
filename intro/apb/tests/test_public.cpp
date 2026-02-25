#include "gtest/gtest.h"
#include "sketch/add.h"

namespace sketch {

TEST(Add, Simple) {
  ASSERT_EQ(Add(2, 3), 5);
}

}  // namespace sketch
