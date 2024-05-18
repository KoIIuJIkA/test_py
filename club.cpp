#include "club.hpp"

bool Club::checkClient(const Table::name_t &name) const {
  return std::any_of(tables_.begin(), tables_.end(),
                     [&name](const Table &x) { return x.client_ == name; });
}

bool Club::checkTable(const size_t i) { return tables_.at(i).free_; }
bool Club::checkFreeTable() {
  return std::any_of(tables_.begin(), tables_.end(),
                     [](const Table &x) { return x.free_ == true; });
}

[[nodiscard]] lcb_ERROR parseEvent(std::ifstream *file, Club *club,
                                   std::set<std::string> *queue) {
  for (std::string line; std::getline(*file, line);) {
    std::pair<uint16_t, uint16_t> time;
    {  // parse time.
      std::string hour, minute;
      bool flag = false;
      if (!(std::all_of(line.begin(), line.begin() + 5,
                        [&hour, &minute, &flag](char x) {  // XX:XX
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
        std::cout << line << std::endl;
        return lcb_ERROR::BAD_CONVERSATION;
      }

      time = std::make_pair(std::stol(hour), std::stol(minute));

      if (time.first < club->open_.first ||
          (time.first == club->open_.first &&
           time.second < club->open_.second)) {
        std::cout << line << std::endl;
        printNotOpen(time);
        continue;
      }
    }  // end parse time.

    uint16_t id = 100;
    size_t posLeft = 6;

    {  // parse id.
      size_t posRight = line.find(' ', posLeft);
      if (posLeft == std::string::npos) {
        std::cout << line << std::endl;
        return lcb_ERROR::BAD_ARGUMENT;
      }

      id = std::stol(line.substr(posLeft, posRight - posLeft));
      posLeft = posRight;
    }  // end parse id.

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
        std::cout << line << std::endl;
        return lcb_ERROR::BAD_ARGUMENT;
    }
  }

  return lcb_ERROR::SUCCESS;
}

[[nodiscard]] lcb_ERROR parseName(std::string *name, const std::string &line,
                                  size_t posLeft) {
  ++posLeft;

  {  // parse name.
    size_t posRight = line.find(' ', posLeft + 1);
    if (posRight != std::string::npos) {
      std::cout << line << std::endl;
      return lcb_ERROR::BAD_ARGUMENT;
    }
    *name = line.substr(posLeft, posRight - posLeft);
  }  // end parse name.

  return lcb_ERROR::SUCCESS;
}

void arrivedCustomer(const Club &club, std::set<std::string> *queue,
                     const std::string &line, size_t posLeft,
                     const std::pair<uint16_t, uint16_t> &time) {
  std::string name;
  check(parseName(&name, line, posLeft), "parse name");

  // check by exist in club.
  if (club.checkClient(name) || queue->contains(name)) {
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

  {  // parse name and table.
    ++posLeft;
    size_t posRight = line.find(' ', posLeft + 1);
    name = line.substr(posLeft, posRight - posLeft);
    posLeft = posRight;

    ++posLeft;
    posRight = line.find(' ', posLeft + 1);
    if (posRight != std::string::npos) {
      std::cout << line << std::endl;
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
      std::cout << line << std::endl;
      return lcb_ERROR::BAD_ARGUMENT;
    }
  }  // end parse name and table.

  if (club->checkTable(n)) {
    club->tables_[n].free_ = false;
    club->tables_[n].client_ = name;
    club->tables_[n].startTime_ = time;
  } else {
    printTime(time);
    std::cout << " 13 PlaceIsBusy\n";
    return lcb_ERROR::SUCCESS;
  }

  queue->erase(name);
  return lcb_ERROR::SUCCESS;
}

void waitCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time) {
  std::string name;
  check(parseName(&name, line, posLeft), "parse name");

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
  check(parseName(&name, line, posLeft), "parse name");

  if (!(club->checkClient(name) || queue->contains(name))) {
    printTime(time);
    std::cout << " ClientUnknown\n";
    return;
  }

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

      // club->tables_[n].allTime_.first +=
      //     time.first - club->tables_[n].startTime_.first;

      if (time.second >= club->tables_[n].startTime_.second) {
        club->tables_[n].allTime_.second +=
            time.second - club->tables_[n].startTime_.second;

        if (club->tables_[n].allTime_.second >= 60) {
          club->tables_[n].allTime_.second -= 60;
          club->tables_[n].allTime_.first += 1;
        }

        club->tables_[n].allTime_.first +=
            time.first - club->tables_[n].startTime_.first;
      } else {
        club->tables_[n].allTime_.second +=
            60 + time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first +=
            time.first - club->tables_[n].startTime_.first - 1;
      }

      club->tables_[n].income_ +=
          club->price_ *
          (time.first - club->tables_[n].startTime_.first +
           (time.second >= club->tables_[n].startTime_.second ? 1 : 0));
    }

    {
      if (!queue->empty()) {
        club->tables_[n].client_ = *(*queue).begin();
        club->tables_[n].startTime_.first = time.first;
        club->tables_[n].startTime_.second = time.second;
        club->tables_[n].free_ = false;
        printTime(time);
        std::cout << " 12 " << *(*queue).begin() << " " << n << std::endl;
        queue->erase(*(*queue).begin());
        // std::cout << "club->tables_[n].startTime_ = "
        //           << club->tables_[n].startTime_.first << ":"
        //           << club->tables_[n].startTime_.second
        //           << ", club->tables_[n].allTime_ = "
        //           << printTime(club->tables_[n].allTime_) << "\n";
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

  delete queue;
  delete club;

  return lcb_ERROR::SUCCESS;
}

void flushClub(Club *club) {
  auto time = club->close_;
  for (size_t n = 1; n < club->tables_.size(); ++n) {
    if (club->tables_[n].free_ == false) {
      printTime(time);
      std::cout << " 11 ";

      if (time.second >= club->tables_[n].startTime_.second) {
        club->tables_[n].allTime_.second +=
            time.second - club->tables_[n].startTime_.second;

        if (club->tables_[n].allTime_.second >= 60) {
          club->tables_[n].allTime_.second -= 60;
          club->tables_[n].allTime_.first += 1;
        }

        club->tables_[n].allTime_.first +=
            time.first - club->tables_[n].startTime_.first;
      } else {
        club->tables_[n].allTime_.second +=
            60 + time.second - club->tables_[n].startTime_.second;
        club->tables_[n].allTime_.first +=
            time.first - club->tables_[n].startTime_.first - 1;
      }

      club->tables_[n].income_ +=
          club->price_ *
          (time.first - club->tables_[n].startTime_.first +
           (time.second >= club->tables_[n].startTime_.second ? 1 : 0));

      std::cout << club->tables_[n].client_ << std::endl;
    }
  }
}

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
    std::cout << line << std::endl;
    return lcb_ERROR::BAD_CONVERSATION;  // not int in the first line.
  }

  Club::time_t open, close;
  {
    std::getline(*file, line);
    if (line.length() != 11) {
      std::cout << line << std::endl;
      return lcb_ERROR::BAD_CONVERSATION;
    }

    // catch "1x:11 21:21".
    if (!(std::all_of(line.begin(), line.end(), [](char x) {
          return (x >= '0' && x <= '9') || x == ':' || x == ' ';
        }))) {
      std::cout << line << std::endl;
      return lcb_ERROR::BAD_CONVERSATION;
    }

    try {
      uint16_t first, second;
      first = std::stol(line.substr(0, 2));
      second = std::stol(line.substr(3, 2));
      open = std::make_pair(first, second);

      first = std::stol(line.substr(6, 2));
      second = std::stol(line.substr(9, 2));
      close = std::make_pair(first, second);
    } catch (...) {
      std::cout << line << std::endl;
      return lcb_ERROR::BAD_CONVERSATION;  // bad time format in the second
                                           // line.
    }
  }

  std::getline(*file, line);
  uint16_t price;
  try {
    price = std::stol(line);
  } catch (...) {
    std::cout << line << std::endl;
    return lcb_ERROR::BAD_CONVERSATION;  // not int in the first line.
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

void check(lcb_ERROR err, const std::string_view &msg) {
  if (err != lcb_ERROR::SUCCESS) {
    std::cerr << "[ERROR] " << msg << "\n";
    exit(EXIT_FAILURE);
  }
}
