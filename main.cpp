#include "club.hpp"

int main(int argc, char **argv) {
  {
    std::string_view file;
    check(getFile(argc, argv, &file), "enter file.txt (*.txt)");
    check(parseFile(file), "parse file");
  }

  return 0;
}
