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

SOURCES := main.cpp \
	include/database/items.cpp \
	include/database/peer.cpp \
	include/database/world.cpp \
	include/network/packet.cpp \
	include/network/compress.cpp \
	include/network/enet_impl.cpp \
	include/event_type/type_receive.cpp \
	include/event_type/type_disconnect_timeout.cpp \
	include/state/punch.cpp \
	include/state/pickup.cpp \
	include/state/movement.cpp \
	include/state/equip.cpp \
	include/action/__action.cpp \
	include/action/logging_in.cpp \
	include/action/refresh_item_data.cpp \
	include/action/enter_game.cpp \
	include/action/dialog_return.cpp \
	include/action/friends.cpp \
	include/action/join_request.cpp \
	include/action/quit_to_exit.cpp \
	include/action/respawn.cpp \
	include/action/input.cpp \
	include/action/drop.cpp \
	include/action/wrench.cpp \
	include/action/quit.cpp \
	include/on/EmoticonDataChanged.cpp \
	include/on/Action.cpp \
	include/on/RequestWorldSelectMenu.cpp \
	include/on/NameChanged.cpp \
	include/commands/__command.cpp \
	include/commands/find.cpp \
	include/commands/warp.cpp \
	include/commands/info.cpp \
	include/tools/randomizer.cpp
	
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
PCH := .make/pch.gch

all: .make $(PCH) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LIBS)

.make :
	@mkdir -p .make

$(PCH): include/pch.hpp | .make
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

%.o: %.cpp $(PCH) | .make
	$(CXX) $(CXXFLAGS) -c $< -o $@
	@mv $*.d .make/

-include $(DEPS)

clean:
	rm -rf $(OBJECTS) .make $(TARGET) $(PCH)