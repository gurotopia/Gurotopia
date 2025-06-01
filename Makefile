CXX = g++
CXXFLAGS = -std=c++23 -g -Iinclude -MMD -MP

LIBS := -L./include
TARGET := main.exe
ifeq ($(OS),Windows_NT)
    LIBS += -lws2_32 -lwinmm -lmimalloc_32
else
    LIBS += -lmimalloc
    TARGET := main.out
endif

PCH := .make/pch.gch

all: .make $(PCH) $(TARGET)

OBJECTS := $(patsubst %.cpp,%.o,main.cpp $(wildcard include/**/*.cpp))
$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LIBS)

.make :
	@mkdir -p .make

$(PCH): include/pch.hpp | .make
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

%.o: %.cpp $(PCH) | .make
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@mv $*.d .make/

-include $(OBJECTS:.o=.d)

clean:
	rm -rf $(OBJECTS) .make $(TARGET) $(PCH)