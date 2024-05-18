#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
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

  /// @brief Checks if there is a client in the club.
  /// @param name Client's name.
  /// @return True if he is in the hall, else return false.
  bool checkClient(const Table::name_t &name) const;

  /// @brief Checks if there is a client on the table.
  /// @param i Table number.
  /// @return True if he is on the table, else return false.
  bool checkTable(const size_t i);

  /// @brief Shows if there are any tables available.
  /// @return True if there is an empty table, else false.
  bool checkFreeTable();

  table_t tables_;
  time_t open_;
  time_t close_;
  money_t price_;
};

// file parse.

/// @brief Used to transfer a file to the program.
/// @param argc Number of command line arguments.
/// @param argv Char arrays of arguments.
/// @param file File name.
/// @return If one file is submitted for input, SUCCESS is returned. Else
/// BAD_ARGUMENT.
[[nodiscard]] lcb_ERROR getFile(int argc, char **argv, std::string_view *file);

/// @brief Implements the file parsing order
/// @param fileName File name.
/// @return CANT_OPEN_FILE or SUCCESS.
[[nodiscard]] lcb_ERROR parseFile(const std::string_view &fileName);

/// @brief Parses the first three lines.
/// @param file File name.
/// @param club Club instance.
/// @return BAD_CONVERSATION or SUCCESS.
[[nodiscard]] lcb_ERROR parseHead(std::ifstream *file, Club *club);

/// @brief Parses strings
/// @param file File name.
/// @param club Club instance.
/// @param queue The queue of visitors.
/// @return BAD_ARGUMENT or SUCCESS.
[[nodiscard]] lcb_ERROR parseEvent(std::ifstream *file, Club *club,
                                   std::set<std::string> *queue);

/// @brief Parsing user names.
/// @param name User name.
/// @param line The line of the file that is being parsed.
/// @param posLeft Position after parsing.
/// @return BAD_ARGUMENT or SUCCESS.
[[nodiscard]] lcb_ERROR parseName(std::string *name, const std::string &line,
                                  size_t posLeft);

//
//  Print block.
//

/// @brief Prints the time in time format.
/// @param time The time to print.
void printTime(const std::pair<uint16_t, uint16_t> &time);

/// @brief Prints NotOpenYet and time.
/// @param pair The time to print.
void printNotOpen(const Club::time_t &pair);

/// @brief Prints YouShallNotPass and time.
/// @param pair The time to print.
void printNotPass(const Club::time_t &pair);

//
//  Id block.
//

/// @brief Executes the logic if the client has arrived.
/// @param club Club instance.
/// @param queue The queue of visitors.
/// @param line The line of the file that is being parsed.
/// @param posLeft Position after parsing.
/// @param time Time of event.
void arrivedCustomer(const Club &club, std::set<std::string> *queue,
                     const std::string &line, size_t posLeft,
                     const std::pair<uint16_t, uint16_t> &time);

/// @brief Executes the logic if the client has sat down at the table.
/// @param club Club instance.
/// @param queue The queue of visitors.
/// @param line The line of the file that is being parsed.
/// @param posLeft Position after parsing.
/// @param time Time of event.
/// @return BAD_ARGUMENT or SUCCESS.
[[nodiscard]] lcb_ERROR sitTable(Club *club, std::set<std::string> *queue,
                                 const std::string &line, size_t posLeft,
                                 const Club::time_t &time);

/// @brief Executes the logic if the client is expecting.
/// @param club Club instance.
/// @param queue The queue of visitors.
/// @param line The line of the file that is being parsed.
/// @param posLeft Position after parsing.
/// @param time Time of event.
void waitCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time);

/// @brief Executes the logic if the client leaves the club.
/// @param club Club instance.
/// @param queue The queue of visitors.
/// @param line The line of the file that is being parsed.
/// @param posLeft Position after parsing.
/// @param time Time of event.
void leftCustomer(Club *club, std::set<std::string> *queue,
                  const std::string &line, size_t posLeft,
                  const Club::time_t &time);

/// @brief Displays information about income.
/// @param club Club instance.
void clubEndInfo(Club *club);

/// @brief Performs the logic of closing the club.
/// @param club Club instance.
void flushClub(Club *club);

/// @brief Сatching errors function.
/// @param err Type of error.
/// @param msg Custom error message.
void check(lcb_ERROR err, const std::string_view &msg);
