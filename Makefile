TARGET      := calcana
SRC_FILES   := main.cpp Evaluate.cpp Cell.cpp MyTable.cpp
OBJECTS     := $(SRC_FILES:.cpp=.o)

CXX            := g++
CPPVERSION     := -std=c++17
CXXFLAGS_DEBUG := -g
# Suppress unused-parameter warnings
CXXFLAGS_WARN  := -Wall -Wextra -Wunreachable-code -Wshadow -Wpedantic -Wno-unused-parameter

FLTK_CXXFLAGS := $(shell fltk-config --cxxflags)
FLTK_LDFLAGS  := $(shell fltk-config --ldflags) $(shell fltk-config --use-images)

%.o: %.cpp
	$(CXX) $(CPPVERSION) $(CXXFLAGS_DEBUG) $(CXXFLAGS_WARN) \
	      $(FLTK_CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CXX) $(CPPVERSION) $(CXXFLAGS_DEBUG) $(FLTK_LDFLAGS) \
	      -o $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS)