# Simple Makefile for File Manager
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = SimpleFileManager
SRC = main.cpp
OBJ = $(SRC:.cpp=.o)

# Platform-specific flags
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    CXXFLAGS += -D_WIN32
else
    CXXFLAGS += -DUNIX
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean run install uninstall