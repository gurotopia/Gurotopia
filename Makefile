CXX = g++
CXXFLAGS = -std=c++2b -g -Iinclude -MMD -MP -MF $(BUILD_DIR)/$*.d

LIBS := -L./include/enet/lib

BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    LIBS += -lssl -lcrypto -lenet_32 -lws2_32 -lwinmm -lsqlite3
    TARGET_NAME := main.exe
else
    LIBS += -lssl -lcrypto -lenet -lsqlite3
    TARGET_NAME := main.out
endif

TARGET := $(TARGET_NAME)
PCH := $(BUILD_DIR)/pch.gch

all: $(BUILD_DIR) $(PCH) $(TARGET)

SOURCES := main.cpp $(wildcard include/**/*.cpp) $(wildcard include/**/**/*.cpp) # todo
OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LIBS)

$(BUILD_DIR) :
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/include/action
	@mkdir -p $(BUILD_DIR)/include/action/dialog_return
	@mkdir -p $(BUILD_DIR)/include/commands
	@mkdir -p $(BUILD_DIR)/include/database
	@mkdir -p $(BUILD_DIR)/include/event_type
	@mkdir -p $(BUILD_DIR)/include/https
	@mkdir -p $(BUILD_DIR)/include/on
	@mkdir -p $(BUILD_DIR)/include/packet
	@mkdir -p $(BUILD_DIR)/include/state
	@mkdir -p $(BUILD_DIR)/include/tools

$(PCH): include/pch.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(PCH) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(BUILD_DIR)
