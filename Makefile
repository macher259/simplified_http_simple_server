CC = g++
CPPFLAGS = -std=c++17 -DDEBUG -Wall -Werror -Wextra -pedantic

all: serwer

serwer: main.cpp TCPServer.cpp CommunicationManager.cpp ResourceManager.cpp
	$(CC) $(CPPFLAGS) -o $@ $^ -lstdc++fs

.PHONY: all clean

clean:
	rm serwer
