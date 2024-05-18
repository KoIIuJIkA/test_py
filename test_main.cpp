#include <gtest/gtest.h>

TEST(PARSE, head) { std::cout << "Wtf\n"; }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
