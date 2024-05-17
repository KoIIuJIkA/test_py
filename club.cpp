#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string_view>
#include <vector>

enum class lcb_ERROR {
  SUCCESS,
  BAD_ARGUMENT,
  CANT_OPEN_FILE,
  BAD_CONVERSATION
};

struct Table final {
  using money_t = uint16_t;
  using name_t = std::string;

  Table() : income_(0), free_(true), client_(), startTime_(), allTime_() {}

  money_t income_;
  bool free_;
  name_t client_;
  std::pair<uint16_t, uint16_t> startTime_;
  std::pair<uint16_t, uint16_t> allTime_;
};

struct Club final {
  using table_t = std::vector<Table>;
  using time_t = std::pair<uint16_t, uint16_t>;
  using money_t = uint16_t;

  Club(size_t n, time_t open, time_t close, money_t price)
      : tables_{n + 1}, open_{open}, close_(close), price_(price) {}
  Club &operator=(const Club &) = delete;
  Club(Club &&) = delete;
  Club &&operator=(Club &&) = delete;
  ~Club() = default;

  bool checkClient(const Table::name_t &name) const {
    return std::any_of(tables_.begin(), tables_.end(),
                       [&name](Table x) { return x.client_ == name; });
  }

  bool checkTable(const size_t i) { return tables_.at(i).free_; }
  bool checkFreeTable() {
    return std::any_of(tables_.begin(), tables_.end(),
                       [](Table x) { return x.free_ == true; });
  }

  table_t tables_;
  time_t open_;
  time_t close_;
  money_t price_;
};

std::ostream &operator<<(std::ostream &os, const Club &club) {
  os << "tables_.size() = " << club.tables_.size()
     << ", open_ = " << club.open_.first << " " << club.open_.second
     << ", close_ = " << club.close_.first << " " << club.close_.first
     << ", price_ = " << club.price_ << std::endl;
  return os;
}

void check(lcb_ERROR err, const std::string_view &msg);

// file parse.
[[nodiscard]] lcb_ERROR getFile(int argc, char **argv, std::string_view *file);
[[nodiscard]] lcb_ERROR parseFile(const std::string_view &fileName);
[[nodiscard]] lcb_ERROR parseHead(std::ifstream *file, Club *club);

// print.
void printTime(const std::pair<uint16_t, uint16_t> &time);
void printNotOpen(const Club::time_t &pair);
void printNotPass(const Club::time_t &pair);

// id's.
void arrivedCustomer(const Club &club, std::set<std::string> *queue,
                     const std::string &line, size_t posLeft,
                     const std::pair<uint16_t, uint16_t> &time);

lcb_ERROR sitTable(Club *club, std::set<std::string> *queue,
                   const std::string &line, size_t posLeft,
                   const Club::time_t &time);

void waitCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time);

void leftCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time);
void clubEndInfo(Club *club);
void flushClub(Club *club);

int main(int argc, char **argv) {

  {
    std::string_view file;
    check(getFile(argc, argv, &file), "enter file.txt (*.txt)");
    check(parseFile(file), "parse file");
  }

  return 0;
}

[[nodiscard]] lcb_ERROR parseEvent(std::ifstream *file, Club *club,
                                   std::set<std::string> *queue) {
  for (std::string line; std::getline(*file, line);) {
    std::pair<uint16_t, uint16_t> time;
    { // parse time.
      std::string hour, minute;
      bool flag = false;
      if (!(std::all_of(line.begin(), line.begin() + 5,
                        [&hour, &minute, &flag](char x) { // XX:XX
                          if (!flag && x == ':') {
                            flag = true;
                            return true;
                          }
                          if (!flag) {
                            hour += x;
                          } else {
                            minute += x;
                          }

                          return (x >= '0' && x <= '9') || x == ':';
                        }))) {
        std::cout << line << std::endl; // task cout. ???
        return lcb_ERROR::BAD_CONVERSATION;
      }

      time = std::make_pair(std::stol(hour), std::stol(minute));

      if (time.first < club->open_.first ||
          (time.first == club->open_.first &&
           time.second < club->open_.second)) {
        std::cout << line << std::endl; // task cout. ???
        printNotOpen(time);
        continue;
      }

      if (time.first == club->close_.first &&
          time.second == club->close_.second) {
        //  213r12 123 123 123 12 3123 123 123 123 123 12 313 123 1 123 123 1123
      }
    } // end parse time.

    uint16_t id = 100;
    size_t posLeft = 6;
    // for (size_t i = 0; i < line.length(); ++i) {
    //   std::cout << "line[i] = " << line[i] << ", i = " << i << "\n";
    // }

    { // parse id.
      size_t posRight = line.find(' ', posLeft);
      if (posLeft == std::string::npos) {
        return lcb_ERROR::BAD_ARGUMENT;
      }

      id = std::stol(line.substr(posLeft, posRight - posLeft));
      // std::cout << "\n\nposLeft = " << posLeft << ", posRight = " << posRight
      //           << ", id = " << id << "\n\n";
      posLeft = posRight;
    } // end parse id.

    switch (id) {
    case 1:
      std::cout << line << std::endl;
      arrivedCustomer(*club, queue, line, posLeft, time);
      break;
    case 2:
      std::cout << line << std::endl;
      check(sitTable(club, queue, line, posLeft, time), "sit table");
      break;
    case 3:
      std::cout << line << std::endl;
      waitCustomer(club, queue, line, posLeft, time);
      break;
    case 4:
      std::cout << line << std::endl;
      leftCustomer(club, queue, line, posLeft, time);
      break;

    default:
      return lcb_ERROR::BAD_ARGUMENT;
    }
  }

  return lcb_ERROR::SUCCESS;
}

[[nodiscard]] lcb_ERROR parseName(std::string &name, const std::string &line,
                                  size_t posLeft) {
  ++posLeft;

  { // parse name.
    size_t posRight = line.find(' ', posLeft + 1);
    if (posRight != std::string::npos) {
      return lcb_ERROR::BAD_ARGUMENT;
    }
    name = line.substr(posLeft, posRight - posLeft);
    // std::cout << "name = " << name << ", posLeft = " << posLeft << std::endl;

  } // end parse name.

  return lcb_ERROR::SUCCESS;
}

void arrivedCustomer(const Club &club, std::set<std::string> *queue,
                     const std::string &line, size_t posLeft,
                     const std::pair<uint16_t, uint16_t> &time) {
  std::string name;
  check(parseName(name, line, posLeft), "parse name");

  // check by exist in club.
  if (club.checkClient(name) || queue->contains(name)) { //  not tested  //sd
    printNotPass(time);
    return;
  }

  queue->insert(name);
}

lcb_ERROR sitTable(Club *club, std::set<std::string> *queue,
                   const std::string &line, size_t posLeft,
                   const Club::time_t &time) {
  std::string name;
  std::size_t n;

  { // parse name and table.
    ++posLeft;
    size_t posRight = line.find(' ', posLeft + 1);
    name = line.substr(posLeft, posRight - posLeft);
    posLeft = posRight;

    ++posLeft;
    posRight = line.find(' ', posLeft + 1);
    if (posRight != std::string::npos) {
      return lcb_ERROR::BAD_ARGUMENT;
    }

    if (!(club->checkClient(name) || queue->contains(name))) {
      printTime(time);
      std::cout << " ClientUnknown\n";
      return lcb_ERROR::SUCCESS;
    }

    try {
      n = std::stol(line.substr(posLeft, posRight - posLeft));
    } catch (...) {
      return lcb_ERROR::BAD_ARGUMENT;
    }
  } // end parse name and table.

  if (club->checkTable(n)) {
    club->tables_[n].free_ = false;
    club->tables_[n].client_ = name;
    club->tables_[n].startTime_ = time;

    // std::cout << "\n\nSIT table " << "client_ = " << club->tables_[n].client_
    //           << ", club->tables_[n].startTime_ = "
    //           << club->tables_[n].startTime_.first << ":"
    //           << club->tables_[n].startTime_.second << "\n";
  } else {
    printTime(time);
    std::cout << " PlaceIsBusy\n";
    return lcb_ERROR::SUCCESS;
  }

  // std::cout << "club->tables_[n].client_ = " << club->tables_[n].client_
  //           << "\n";

  queue->erase(name);

  return lcb_ERROR::SUCCESS;
}

void waitCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time) {
  std::string name;
  check(parseName(name, line, posLeft), "parse name");

  if (club->checkFreeTable() && queue->contains(name)) {
    printTime(time);
    std::cout << " 13 ICanWaitNoLonger!\n";
    return;
  }

  if (club->tables_.size() < queue->size()) {
    printTime(time);
    std::cout << " 11 " << name << std::endl;
    queue->erase(name);
  }
}

void leftCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time) {
  std::string name;
  check(parseName(name, line, posLeft), "parse name");

  // std::cout << "\n\nclub->checkClient(name) = " << club->checkClient(name)
  //           << ", queue->contains(name) = " << queue->contains(name) <<
  //           "\n\n";

  if (!(club->checkClient(name) || queue->contains(name))) {
    printTime(time);
    std::cout << " ClientUnknown\n";
    return;
  }

  // std::cout << "club->checkClient(name) = " << club->checkClient(name)
  //           << std::endl;

  // for (size_t i = 0; i < club->tables_.size(); ++i) {
  //   std::cout << club->tables_[i].client_ << " ";
  // }

  if (club->checkClient(name)) {
    size_t n = 0;
    for (size_t i = 0; i < club->tables_.size(); ++i) {
      if (club->tables_[i].client_ == name) {
        n = i;
        break;
      }
    }

    {
      club->tables_[n].client_ = "";
      club->tables_[n].free_ = true;

      club->tables_[n].allTime_.first =
          time.first - club->tables_[n].startTime_.first;

      if (time.second >= club->tables_[n].startTime_.second) {
        club->tables_[n].allTime_.second =
            time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first =
            time.first - club->tables_[n].startTime_.first;
      } else {
        club->tables_[n].allTime_.second =
            60 + time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first =
            time.first - club->tables_[n].startTime_.first - 1;
      }

      club->tables_[n].income_ =
          club->price_ *
          (time.first - club->tables_[n].startTime_.first +
           (time.second >= club->tables_[n].startTime_.second ? 1 : 0));
    }

    // std::cout << "\nqueue->empty() = " << queue->empty() << "\n";
    // for (auto i : *queue) {
    //   std::cout << i << " ";
    // }
    // std::cout << "\nend\n";

    {
      if (!queue->empty()) {
        club->tables_[n].client_ = *(*queue).begin();

        club->tables_[n].startTime_.first = time.first;
        club->tables_[n].startTime_.second = time.second;
        club->tables_[n].free_ = false;
        printTime(time);
        std::cout << " 12 " << *(*queue).begin() << " " << n << std::endl;
        queue->erase(*(*queue).begin());

        // std::cout << "\n\nSIT table "
        //           << "client_ = " << club->tables_[n].client_
        //           << ", club->tables_[" << n
        //           << "].startTime_ = " << club->tables_[n].startTime_.first
        //           << ":" << club->tables_[n].startTime_.second
        //           << ", club->tables_[n].allTime = "
        //           << club->tables_[n].allTime_.first << ":"
        //           << club->tables_[n].allTime_.second << "\n";
      }
    }
  }
}

[[nodiscard]] lcb_ERROR parseFile(const std::string_view &fileName) {
  std::ifstream file(fileName.data());
  if (!file) {
    return lcb_ERROR::CANT_OPEN_FILE;
  }

  auto queue = new std::set<std::string>;
  Club *club = static_cast<Club *>(::operator new(sizeof(Club)));

  check(parseHead(&file, club), "parseHead");
  check(parseEvent(&file, club, queue), "parse event");
  flushClub(club);
  printTime(club->close_);
  std::cout << std::endl;
  clubEndInfo(club);

  return lcb_ERROR::SUCCESS;
}

void flushClub(Club *club) {
  auto time = club->close_;
  for (size_t n = 1; n < club->tables_.size(); ++n) {
    if (club->tables_[n].free_ == false) {
      printTime(time);
      std::cout << " 11 ";

      club->tables_[n].allTime_.first =
          time.first - club->tables_[n].startTime_.first;

      if (time.second >= club->tables_[n].startTime_.second) {
        club->tables_[n].allTime_.second =
            time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first =
            time.first - club->tables_[n].startTime_.first;
      } else {
        club->tables_[n].allTime_.second =
            60 + time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first =
            time.first - club->tables_[n].startTime_.first - 1;
      }

      club->tables_[n].income_ =
          club->price_ *
          (time.first - club->tables_[n].startTime_.first +
           (time.second >= club->tables_[n].startTime_.second ? 1 : 0));

      std::cout << club->tables_[n].client_ << std::endl;
    }
  }
}

// money_t income_;
// bool free_;
// name_t client_;
// std::pair<uint16_t, uint16_t> startTime_;
// std::pair<uint16_t, uint16_t> allTime_;

void clubEndInfo(Club *club) {
  for (size_t i = 1; i < club->tables_.size(); ++i) {
    std::cout << i << " " << club->tables_[i].income_ << " ";
    printTime(club->tables_[i].allTime_);
    std::cout << std::endl;
  }
}

[[nodiscard]] lcb_ERROR parseHead(std::ifstream *file, Club *club) {
  std::string line;
  std::getline(*file, line);

  size_t n;
  try {
    n = std::stol(line);
  } catch (...) {
    return lcb_ERROR::BAD_CONVERSATION; // not int in the first line.
  }

  Club::time_t open, close;
  {
    std::getline(*file, line);
    if (line.length() != 11) {
      return lcb_ERROR::BAD_CONVERSATION;
    }

    // catch "1x:11 21:21".
    if (!(std::all_of(line.begin(), line.end(), [](char x) {
          return (x >= '0' && x <= '9') || x == ':' || x == ' ';
        }))) {
      return lcb_ERROR::BAD_CONVERSATION;
    }

    uint16_t first, second;
    try {
      first = std::stol(line.substr(0, 2));
      second = std::stol(line.substr(3, 2));
      open = std::make_pair(first, second);

      first = std::stol(line.substr(6, 2));
      second = std::stol(line.substr(9, 2));
      close = std::make_pair(first, second);
    } catch (...) {
      return lcb_ERROR::BAD_CONVERSATION; // bad time format in the second line.
    }
  }

  std::getline(*file, line);
  uint16_t price;
  try {
    price = std::stol(line);
  } catch (...) {
    return lcb_ERROR::BAD_CONVERSATION; // not int in the first line.
  }

  new (club) Club(n, open, close, price);

  printTime(club->open_);
  std::cout << "\n";

  return lcb_ERROR::SUCCESS;
}

[[nodiscard]] lcb_ERROR getFile(int argc, char **argv, std::string_view *file) {
  if (argc != 2) {
    return lcb_ERROR::BAD_ARGUMENT;
  }

  if (!(std::strlen(argv[1]) > 4)) {
    return lcb_ERROR::BAD_ARGUMENT;
  }

  *file = argv[1];

  return lcb_ERROR::SUCCESS;
}

//
// PRINT
//

void printNotPass(const Club::time_t &pair) {
  printTime(pair);
  std::cout << " 13 YouShallNotPass\n";
}

void printTime(const std::pair<uint16_t, uint16_t> &time) {
  if (time.first > 9) {
    std::cout << time.first << ":";
  } else {
    std::cout << "0" << time.first << ":";
  }

  if (time.second > 9) {
    std::cout << time.second;
  } else {
    std::cout << "0" << time.second;
  }
}

void printNotOpen(const Club::time_t &pair) {
  printTime(pair);
  std::cout << " 13 NotOpenYet\n";
}

//
// CHECK
//

void check(lcb_ERROR err, const std::string_view &msg) {
  if (err != lcb_ERROR::SUCCESS) {
    std::cerr << "[ERROR] " << msg << "\n";
    exit(EXIT_FAILURE);
  }
}

// for (size_t i = 0; i < line.size(); ++i) {
//   std::cout << "line[i] = " << line[i] << ", i = " << i << "\n";
// }
