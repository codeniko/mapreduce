#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char **argv)
{
	extern char *optarg; 
//	extern int optind; 
	string application = ""; /*[wordcount, sort]*/
	string interface= ""; /*[procs, threads]*/
	int c, numMap = 0, numReduce = 0;
	string infile ="", outfile="";
	string usage = "mapred -a [wordcount, sort] -i [procs, threads] -m num_maps -r num_reduces infile outfile";

	if (argc != 11) {
		cout << usage << endl;
		return 1;
	}

	while ((c = getopt(argc, argv, "a:i:m:r:")) != -1) {
		switch (c) {
			case 'a': application = optarg; break;
			case 'i': interface = optarg; break;
			case 'm': numMap = atoi(optarg); break;
			case 'r': numReduce = atoi(optarg); break;
			case '?': cout << usage << endl; return 1;
		}
	}
	infile = argv[9];
	outfile = argv[10];

	/*check if all arguments set*/
	if (numMap == 0 || numReduce == 0 || infile.size()==0 || outfile.size()==0 || application.size()==0 || interface.size()==0) {
		cout << usage << endl;
		return 1;
	}

	/*split input file into map number of files*/
	pid_t pid = fork();
	if (pid == 0) {
		char numbuffer[11];
		numbuffer[10] = '\0';
		sprintf(numbuffer, "%d", numMap);
		execlp("/bin/sh", "/bin/sh", "./split.sh", infile.c_str(), numbuffer, NULL);
	} else
		wait(0);



	return 0;
}
