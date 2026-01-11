CXX := g++
CXXFLAGS := -std=c++2b -g -Iinclude -MMD -MP
LIBS := -L./include/enet/lib -lssl -lcrypto -lsqlite3

BUILD_DIR := build

ifeq ($(OS),Windows_NT)
	LIBS += -lenet_32 -lws2_32 -lwinmm
	OUTPUT := main.exe
else
	LIBS += -lenet
	OUTPUT := main.out
endif

all: $(OUTPUT)

SOURCES := main.cpp \
		$(wildcard include/*.cpp) \
		$(wildcard include/**/*.cpp) \
		$(wildcard include/**/**/*.cpp)

OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJECTS:.o=.d)

$(OUTPUT): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LIBS)

$(BUILD_DIR)/pch.gch: include/pch.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -Iinclude -x c++-header $< -o $@

$(BUILD_DIR)/%.o: %.cpp $(BUILD_DIR)/pch.gch | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR)
