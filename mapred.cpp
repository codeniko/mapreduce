#include "mapred.hpp"

using namespace std;

int main(int argc, char **argv)
{
	extern char *optarg; 
	Args args;
	int c;
	string usage = "./mapred -a [wordcount, sort] -i [procs, threads] -m num_maps -r num_reduces infile outfile";

	if (argc != 11) {
		cout << usage << endl;
		return 1;
	}

	while ((c = getopt(argc, argv, "a:i:m:r:")) != -1) {
		switch (c) {
			case 'a': args.application = optarg; break;
			case 'i': args.interface = optarg; break;
			case 'm': args.numMap = atoi(optarg); break;
			case 'r': args.numReduce = atoi(optarg); break;
			case '?': cout << usage << endl; return 1;
		}
	}
	args.infile = argv[9];
	args.outfile = argv[10];

	/*check if all arguments set*/
	if (args.numMap == 0 || args.numReduce == 0 || args.infile.size()==0 || args.outfile.size()==0 || args.application.size()==0 || args.interface.size()==0) {
		cout << usage << endl;
		return 1;
	}

	/*split input file into map number of files*/
	pid_t pid = fork();
	if (pid == 0) {
		char numbuffer[11];
		numbuffer[10] = '\0';
		sprintf(numbuffer, "%d", args.numMap);
		execlp("/bin/sh", "/bin/sh", "./split.sh", args.infile.c_str(), numbuffer, NULL);
	} else
		wait(0);

	pthread_mutex_init(&lock, 0);

	if (args.application == "wordcount")
		wc_mapreduce(args);

	pthread_mutex_destroy(&lock);
	return 0;
}

void wc_mapreduce(Args &args)
{
	vector<pthread_t> threads; /*holds all thread IDs*/
	vector< vector<WC_Node> > threadwork; /* holds all work of threads*/

	/*create threads, assign threads a file, and send them off to work*/
	for (int i = 0; i < args.numMap; i++) {
		WC_Map_Thread_Args *threadArgs = new WC_Map_Thread_Args;
		string partition = args.infile + "." + to_string(i);
		memcpy(threadArgs->file, partition.c_str(), partition.size()+1);

		threadArgs->fileContent = new list<string>;
		ifstream file;
		file.open(threadArgs->file);
		if (!file.is_open()) {
			cerr << "ERROR: unable to open file '" << threadArgs->file << "'";
			exit(1);
		}
		string line;
		while (getline(file, line))
			threadArgs->fileContent->push_back(line);
		file.close();

		
		vector<WC_Node> node_v;
		threadwork.push_back(node_v);
		threadArgs->outNodes = &threadwork.back();
		pthread_t thread;
		pthread_create(&thread, 0, wc_map, (void *)threadArgs);
		threads.push_back(thread);
	}
	
	/*wait for each map thread to finish*/
	for (vector<pthread_t>::iterator t_it = threads.begin(); t_it < threads.end(); t_it++)
		pthread_join(*t_it, 0);

	/*merge all work done by threads into one thread*/
	cout << "shit" << endl;
	vector<WC_Node> allwords = threadwork.front();
	for (vector< vector<WC_Node> >::iterator twork_it = threadwork.begin()+1; twork_it < threadwork.end(); twork_it++)
		allwords.insert(allwords.end(), twork_it->begin(), twork_it->end());

	cout << "fuck" << endl;
	sort(allwords.begin(), allwords.end(), WC_Node::compareTo);

	for (vector<WC_Node>::iterator wc_it = allwords.begin(); wc_it < allwords.end(); wc_it++)
		cout << wc_it->key << ",  " << wc_it->count << endl;
}

void *wc_map(void *arguments) 
{
	WC_Map_Thread_Args *args = (WC_Map_Thread_Args *)arguments;
	/*ifstream infile;
	infile.open(args->file);
	if (!infile.is_open()) {
		cerr << "ERROR: unable to open file '" << args->file << "'";
		pthread_exit(0);
	}

	string line;
	while (getline(infile, line)) {
		transform(line.begin(), line.end(), line.begin(), ::tolower);
		istringstream ss(line);
			do {
				string word;
				ss >> word;
				if (word != "")
					args->outNodes->push_back(WC_Node(word, 1));
			} while (ss);
	}
*/
	//infile.close();

	cout << "Parsing contents for " << args->file << endl;
	for (list<string>::iterator line_it = args->fileContent->begin(); line_it != args->fileContent->end(); line_it++) {
		istringstream ss(*line_it);
		do {
			string word;
			ss >> word;
			transform(word.begin(), word.end(), word.begin(), ::tolower);
			if (word != "") {
				pthread_mutex_lock(&lock);
				args->outNodes->push_back(WC_Node(word, 1));
				pthread_mutex_unlock(&lock);
			}
		} while (ss);
	}
	for (int i = 0; i < args->outNodes->size(); i++)
		cout << (*args->outNodes)[i].key << "   " << (*args->outNodes)[i].count << endl;
	cout << "DONE - Parsing contents for " << args->file << endl;


	return 0;
}

void wc_reduce()
{
}
