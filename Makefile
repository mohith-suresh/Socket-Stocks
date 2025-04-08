# Makefile for EE450 Socket Programming Project

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11

# Targets
all: client serverM serverA serverP serverQ

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

serverM: serverM.cpp
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp

serverA: serverA.cpp
	$(CXX) $(CXXFLAGS) -o serverA serverA.cpp

serverP: serverP.cpp
	$(CXX) $(CXXFLAGS) -o serverP serverP.cpp

serverQ: serverQ.cpp
	$(CXX) $(CXXFLAGS) -o serverQ serverQ.cpp

clean:
	rm -f client serverM serverA serverP serverQ

test: all
	./auto_test.sh

test_manual: all
	./simplified_test.sh
	
menu: all
	./run_tests.sh
