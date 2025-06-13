#
# FILENAME: Ludo/Makefile
#
CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall -g
LIBS = -lncurses

all: server client

server: server.cpp common.h
	$(CXX) $(CXXFLAGS) -o server server.cpp $(LIBS)

client: client.cpp common.h
	$(CXX) $(CXXFLAGS) -o client client.cpp $(LIBS)

clean:
	rm -f server client *.o