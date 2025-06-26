CXX = g++
CXXFLAGS = -std=c++2b -g -Iinclude -Iinclude/ssl -MMD -MP

LIBS := -L./include/enet/lib -L./include/mimalloc/lib -L./include/sqlite/lib -L./include/ssl/openssl/lib -L./include/ssl/crypto/lib
TARGET := main.exe
ifeq ($(OS),Windows_NT)
    LIBS += -lssl_32 -lcrypto_32 -lenet_32 -lws2_32 -ladvapi32 -lcrypt32 -lwinmm -lmimalloc_32 -lsqlite3_32
else
    LIBS += -lssl -lcrypto -lenet -lmimalloc -lsqlite3
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
