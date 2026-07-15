CXX := g++
CXXFLAGS := -std=c++20 -g -Iinclude -MMD -MP
LDLIBS := -L./include/enet/lib -L./include/mysql/lib -lssl -lcrypto -lmariadb

ifeq ($(OS),Windows_NT)
LDLIBS += -lenet_32 -lws2_32 -lwinmm
else # linux
LDLIBS += -lenet
endif

all: main.out

SOURCES := main.cpp \
		$(wildcard include/*.cpp) \
		$(wildcard include/**/*.cpp) \
		$(wildcard include/**/**/*.cpp)

objects := $(SOURCES:%.cpp=build/%.o)

main.out: $(objects)
	$(CXX) $(objects) -o $@ $(LDLIBS)

build/pch.gch: include/pch.hpp | build
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

build/%.o: %.cpp build build/pch.gch
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -include include/pch.hpp -c $< -o $@

build:
	@mkdir -p $@

-include $(objects:.o=.d)

.PHONY : clean
clean :
	-rm -rf build