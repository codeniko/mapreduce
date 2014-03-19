#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <vector>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <list>

#define PARTITION_SIZE 100

using namespace std;

struct Args {
	string application; /*[wordcount, sort]*/
	string interface; /*[procs, threads]*/
	string infile; /*input file*/
	string outfile; /*output file*/
	int numMap; /* number of map workers */
	int numReduce; /*number of reduce workers*/

	Args() {
		application = "";
		interface = "";
		infile = "";
		outfile = "";
		numMap = 0;
		numReduce = 0;
	}
};

/*Word count node*/
struct WC_Node;
struct WC_Node {
	string key;
	int count;

	WC_Node() {}
	WC_Node(string key, int count) {
		this->key = key;
		this->count = count;
	}

	static bool compareTo(WC_Node a, WC_Node b) {
		return a.key < b.key;
	}
};

struct WC_Map_Thread_Args {
	char file[PARTITION_SIZE];
	vector<WC_Node> *outNodes;
	list<string> *fileContent;
};

void wc_mapreduce(Args &);
void *wc_map(void *);
void wc_reduce();

pthread_mutex_t lock;
