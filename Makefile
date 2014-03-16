CFLAGS=-g -std=c++11 -Wall

mapred: mapred.o
	g++ $(CFLAGS) -o mapred mapred.o

mapred.o: mapred.cpp
	g++ $(CFLAGS) -c mapred.cpp

.PHONY: clean
clean:
	rm *.o mapred
