CC = gcc
AR = ar

CFLAGS = -std=gnu2x -Wall -Werror -Wextra -pedantic -O2 -march=native -mtune=native -fPIC

INCLUDES = -Iinclude/

SRC_FILES = $(shell find src/ -name "*.c")
TEST_FILES = $(shell find tests/ -name "*.c")

OBJ_FILES = $(SRC_FILES:src/%.c=build/obj/%.o)

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)
TEST_BUILD_DIR = $(BUILD_DIR)/tests

STATIC_LIB = $(LIB_DIR)/taskio.a
SHARED_LIB = $(LIB_DIR)/taskio.so

$(shell mkdir -p $(OBJ_DIR) $(TEST_BUILD_DIR))

all: $(STATIC_LIB) $(SHARED_LIB) $(TEST_FILES:tests/%.c=$(TEST_BUILD_DIR)/%.out)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(STATIC_LIB): $(OBJ_FILES)
	$(AR) rcs $@ $^

$(SHARED_LIB): $(OBJ_FILES)
	$(CC) -shared -o $@ $^

$(TEST_BUILD_DIR)/%.out: tests/%.c $(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(STATIC_LIB) -o $@

clean:
	rm -rf $(BUILD_DIR)
