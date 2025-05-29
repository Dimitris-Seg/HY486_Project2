# Author: 	Dimitris Segkesser
# Date: 	25/05/2025

EXEC := mpirun
EXECFLAGS := --oversubscribe
NP := 23
N := 3
INSTR_FILE := "./testfiles_hy486/testfile0/loaners_13_libs_9_np_23.txt"

CC := mpicc
CFLAGS := -Wall -Wextra -std=c11 -g -MMD -MP
LDFLAGS :=   # ADD MPI flag here
SRC_DIR := ./src
BUILD_DIR := ./build
TARGET := $(BUILD_DIR)/main
TEST_DIR := ./tests

# Find all .c files recursively
SRC_FILES := $(shell find $(SRC_DIR) -type f -name "*.c")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)  # Generate .d files alongside .o files

.PHONY: all clean

# Default target
all: $(TARGET)

# Create the target executable
$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Compile each .c file into a .o file in /build and generate dependencies
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Include dependency files to track header changes
-include $(DEP_FILES)

# Clean up the build directory
clean:
	rm -rf $(BUILD_DIR)

test1: $(TARGET)
	$(EXEC) $(EXECFLAGS) -np $(NP) $(TARGET) $(INSTR_FILE) $(N)