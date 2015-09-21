GXX=g++
INCLUDES=-Iheaders
CXXFLAGS=-Os -Wall -std=c++11
LDFLAGS=-Wl,--as-needed -Wl,-O1
LIBS=

HEADERS=$(wildcard headers/*.h)
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(SOURCES:src/%.cpp=build/%.o)

.PHONY: all clean

all: ${OBJECTS}

build/%.o: src/%.cpp ${HEADERS}
	${GXX} -c ${INCLUDES} ${CXXFLAGS} $< -o $@

clean:
	rm -rf build/*
