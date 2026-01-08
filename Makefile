# ================================================================
# Memory Simulator Cross-Platform Makefile
# ================================================================

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# OS Detection & Shell Command Configuration
ifeq ($(OS),Windows_NT)
    # Windows (CMD/PowerShell) settings
    MKDIR = if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
    RM = if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)
    TARGET = memsim.exe
    CLEAN_TARGET = if exist $(TARGET) del $(TARGET)
else
    # Linux/macOS settings
    MKDIR = mkdir -p $(BUILD_DIR)
    RM = rm -rf $(BUILD_DIR)
    TARGET = memsim
    CLEAN_TARGET = rm -f $(TARGET)
endif

# Source files
MAIN_SRC = $(SRC_DIR)/main.cpp
CACHE_SRC = $(SRC_DIR)/cache/cache_simulator.cpp
ALLOCATOR_SRC = $(SRC_DIR)/allocator/memory_allocator.cpp
BUDDY_SRC = $(SRC_DIR)/buddy/buddy_allocator.cpp
VM_SRC = $(SRC_DIR)/virtual_memory/virtual_memory_simulator.cpp

# Object files
OBJS = $(BUILD_DIR)/main.o \
       $(BUILD_DIR)/cache_simulator.o \
       $(BUILD_DIR)/memory_allocator.o \
       $(BUILD_DIR)/buddy_allocator.o \
       $(BUILD_DIR)/virtual_memory_simulator.o

# ================================================================
# Main targets
# ================================================================

all: $(BUILD_DIR) $(TARGET)
	@echo "✓ Build complete! Run with: ./$(TARGET)"

$(BUILD_DIR):
	$(MKDIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "✓ Linked $(TARGET)"

# ================================================================
# Compilation rules
# ================================================================

$(BUILD_DIR)/main.o: $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/cache_simulator.o: $(CACHE_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/memory_allocator.o: $(ALLOCATOR_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/buddy_allocator.o: $(BUDDY_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/virtual_memory_simulator.o: $(VM_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ================================================================
# Utility targets
# ================================================================

clean:
	$(RM)
	$(CLEAN_TARGET)
	@echo "✓ Cleaned build files"

rebuild: clean all

run: all
	./$(TARGET)

help:
	@echo "Memory Simulator Build Commands:"
	@echo "  make         - Build the simulator"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make run     - Build and run"

.PHONY: all clean rebuild run help