CFLAGS=-g -std=c++11 -Wall -pthread

mapred: mapred.o
	g++ $(CFLAGS) -o mapred mapred.o

mapred.o: mapred.cpp mapred.hpp
	g++ $(CFLAGS) -c mapred.cpp

.PHONY: clean
clean:
	rm *.o mapred
