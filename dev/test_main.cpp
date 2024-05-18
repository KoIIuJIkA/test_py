#include <gtest/gtest.h>

#include <filesystem>

#include "club.hpp"

namespace fs = std::filesystem;

TEST(PARSE, head) {
  std::string filePath = std::filesystem::current_path();
  filePath += "/data/testHead.txt";
  std::cout << filePath;
  std::ifstream file(filePath);
  Club *club = static_cast<Club *>(::operator new(sizeof(Club)));
  check(parseHead(&file, club), "test parseHead");

  bool flag = true;
  // 3 tables + 1 table for fast access.
  flag = flag && club->tables_.size() == 4;
  flag = flag && club->open_ == static_cast<std::pair<uint16_t, uint16_t>>(
                                    std::make_pair(9, 0));
  flag = flag && club->close_ == static_cast<std::pair<uint16_t, uint16_t>>(
                                     std::make_pair(19, 0));
  flag = flag && club->price_ == 10;

  delete club;
  EXPECT_TRUE(flag);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
