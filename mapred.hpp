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

/*arguments when program invoked*/
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

/*Argument passed to new reduce threads on creation for wordcount*/
struct WC_ReduceStruct {
	bool die; /*flag for master to tell slave to die*/
	bool done; /*flag for slave to tell master it's ready for work*/
	//int tid; /*thread id for testing purpose, remove later*/
	vector<WC_Node> *keyVector; /*key vector (work assignment)*/
	static vector<WC_Node> reduceNodes; /*overall output for all reduce threads*/
	static pthread_cond_t cv_master;
	static pthread_cond_t cv_slave;
	static pthread_mutex_t mtx_master;

	WC_ReduceStruct() {
		die = false;
		done = false;
	}
};
vector<WC_Node> WC_ReduceStruct::reduceNodes;
pthread_cond_t WC_ReduceStruct::cv_master;
pthread_cond_t WC_ReduceStruct::cv_slave;
pthread_mutex_t WC_ReduceStruct::mtx_master;

/*Arguments passed to new Map threads on creation for wordcount*/
struct WC_MapStruct {
	char file[PARTITION_SIZE];
	vector<WC_Node> *threadwork;
	list<string> *fileContent;
};

int wc_mapreduce(Args &);
vector<WC_Node>* wc_getNextKeyVector(vector<WC_Node> &);
void *wc_map(void *);
void *wc_reduce(void *);

static pthread_mutex_t lock;
