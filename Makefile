# Author: 	Dimitris Segkesser
# Date: 	25/05/2025

EXEC := mpirun
EXECFLAGS := --oversubscribe

NP0 := 23
NUM_LIBS0 := 9
#
NP1 := 49
NUM_LIBS1 := 16
#
NP2 := 88
NUM_LIBS2 := 25

INSTR_FILE0 := "./testfiles_hy486/testfile0/loaners_13_libs_9_np_23.txt"
INSTR_FILE1 := "./testfiles_hy486/testfile1/loaners_32_libs_16_np_49.txt"
INSTR_FILE2 := "./testfiles_hy486/testfile2/loaners_62_libs_25_np_88.txt"

CC := mpicc
CFLAGS := -Wall -Wextra -std=c11 -g -MMD -MP
LDFLAGS := -lm  # ADD MPI flag here
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

test0: $(TARGET)
	$(EXEC) $(EXECFLAGS) -np $(NP0) $(TARGET) $(INSTR_FILE0) $(NUM_LIBS0)

test1: $(TARGET)
	$(EXEC) $(EXECFLAGS) -np $(NP1) $(TARGET) $(INSTR_FILE1) $(NUM_LIBS1)	

test2: $(TARGET)
	$(EXEC) $(EXECFLAGS) -np $(NP2) $(TARGET) $(INSTR_FILE2) $(NUM_LIBS2)	