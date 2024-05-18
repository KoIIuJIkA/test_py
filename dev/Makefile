# Name of the target executable.
TARGET := myapp.out
TEST_TARGET := test.out

# Source files.
SOURCES := club.cpp main.cpp
IN_FILE := data/data.txt

# Test files.
TEST_SOURSES := test_main.cpp
TEST_FILE := data/testHead.txt

# Object files.
OBJECTS := $(SOURCES:.cpp=.o)

# Object test files.
TEST_OBJECTS := $(TEST_SOURSES:.cpp=.o)

# Input file.
INPUT := input.txt

# Compiler.
CXX := g++

# Compiler flags.
CXXFLAGS := -std=c++20 -Wall -Werror -Wextra -Wno-unused-variable

# Linker flags.
LDFLAGS := -lgtest -lgtest_main -pthread

# Includes flags.
GTEST_INCLUDE=-I/usr/local/include/googletest/googletest/include

# Valgrind options
VALGRIND_OPTS := --leak-check=full --show-leak-kinds=all --track-origins=yes

# Default make target
all: build

# Build step
build: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Test target build
$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) -o $(TEST_TARGET) $(TEST_OBJECTS) $(filter-out main.o, $(OBJECTS)) $(LDFLAGS)


# Compile step for source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS)  $(GTEST_INCLUDE) -c $< -o $@

# Run step
run: build
	./$(TARGET) $(IN_FILE)

# Run tests
test: build
	./$(TEST_TARGET) $(TEST_FILE)

# Memory check step
memory: build
	valgrind $(VALGRIND_OPTS) ./$(TARGET) $(IN_FILE)

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET)

.PHONY: all build run memory clean