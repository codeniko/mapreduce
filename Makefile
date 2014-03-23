CFLAGS=-g -std=c++0x -Wall -pthread

mapred: mapred.o
	g++ $(CFLAGS) -o mapred mapred.o

mapred.o: mapred.cpp mapred.hpp
	g++ $(CFLAGS) -c mapred.cpp

.PHONY: clean
clean:
	rm -f *.o mapred