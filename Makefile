GXX=g++
INCLUDES=-Iheaders
CXXFLAGS=-Os -Wall -std=c++11 -flto
LDFLAGS=-Wl,--as-needed -Wl,-O1 -flto
LIBS=-lboost_system -lboost_filesystem -lboost_iostreams -lboost_coroutine

HEADERS=$(wildcard headers/*.h)
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(SOURCES:src/%.cpp=build/%.o)

.PHONY: all clean

all: ${OBJECTS} build/server build/client

build/%.o: src/%.cpp ${HEADERS}
	${GXX} -c ${INCLUDES} ${CXXFLAGS} $< -o $@

build/client: build/client.o build/common.o build/communication.o build/file.o build/hash.o build/ui.o
	${GXX} -o $@ ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS}

build/server: build/server.o build/common.o build/communication.o build/file.o build/hash.o build/ui.o
	${GXX} -o $@ ${CXXFLAGS} ${LDFLAGS} $^ ${LIBS}

clean:
	rm -rf build/*
