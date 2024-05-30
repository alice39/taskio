CC = gcc
CFLAGS = -std=gnu2x -Wall -Werror -Wextra -pedantic -O2 -march=native -mtune=native
INCLUDES = -Iinclude/
SRC_FILES = $(shell find src/ -name "*.c")
TEST_FILES = $(shell find tests/ -name "*.c")
BUILD_DIR = build
TEST_BUILD_DIR = $(BUILD_DIR)/tests

$(shell mkdir -p $(BUILD_DIR) $(TEST_BUILD_DIR))

all: $(TEST_FILES:tests/%.c=$(TEST_BUILD_DIR)/%)

$(TEST_BUILD_DIR)/%: tests/%.c $(SRC_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

clean:
	rm -rf $(BUILD_DIR)/*

clean-tests:
	rm -f $(TEST_BUILD_DIR)/*

