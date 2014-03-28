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

	int ret = 0;
	if (args.application == "wordcount" && args.interface == "threads")
		ret = wc_mapreduce(args);
		
	/*if user chose integer sort*/
	else if (args.application == "sort" && args.interface == "threads"){
		ret = sort_mapreduce(args);
	} else {
		cerr << "Sorry, this mapreduce program only does the 'wordcount' and 'sort' applications using 'threads'. Please invoke the program using the correct arguments." << endl;
		system(string("rm -f "+args.infile+".[0-9]*").c_str());
		return 1;
	}

	pthread_mutex_destroy(&lock);

	/*remove the files that we initially split*/
	system(string("rm -f "+args.infile+".[0-9]*").c_str());

	return ret;
}

int sort_mapreduce(Args &args){
	/*
	 * 1) reduce thread needs vector of work done by map threads, have a vector<struct sort_MapStruct>, create a main sorted vector (starts empty, FOR REDUCE to output into)
	 * 2) knowing the amount of map workers you need, read input file one by one, create an instance of struct sort_mapStruct, stick it into vector in step 1, 
	 * 		stick contents of file into sort_mapStruct->fileContents, create thread and pass in sort_mapStruct pointer
	 * 3) map worker sorts the filecontents (vector), main thread still has access to it because of vector in step 1
	 * 4) once all map workers are done, call reduce and give that vector from step 1
	 * 5) reduce vector - 
	 * 6) reduce vector - look at first index of each sort_MapStruct passed, remove lowest of them all, and stick it into vector of step 5
	 * 7) repeat step 6 until all sort_mapStructs passed in are empty
	 * 8) done
	 * 
	 * */
	 /*threads for number of maps given by user*/
	vector<pthread_t> threads;
	/*the final sorted list that will be passed to reduce and ultimately is the one that will be the output*/
	vector<SORT_Node> final_sorted;
	/*vector of each sorted map returned by map*/
	vector<struct SORT_MapStruct> all_maps;
	
	struct SORT_ReduceStruct *reduce_maps = new struct SORT_ReduceStruct;
	reduce_maps->final_sorted = &final_sorted;
	reduce_maps->all_maps = &all_maps;
	
	for (int i = 0; i < args.numMap; i++) {
		struct SORT_MapStruct *threadArgs = new struct SORT_MapStruct;
		char i_str[6];
		sprintf(i_str, "%d", i);
		string partition = args.infile + "." + i_str;
		memcpy(threadArgs->file, partition.c_str(), partition.size()+1);

		threadArgs->fileContent = new vector<int>;
		ifstream file;
		file.open(threadArgs->file);
		if (!file.is_open()) {
			cerr << "ERROR: unable to open file '" << threadArgs->file << "'";
			exit(1);
		}
		string line;
		while (getline(file, line)){
			const char * c = line.c_str();
			int a = atoi(c);
			threadArgs->fileContent->push_back(a);
		}
		file.close();
		

		if (threadArgs->fileContent->empty()) {
			delete threadArgs->fileContent;
			delete threadArgs;
			break;
		}
	/*	for( std::vector<int>::const_iterator i = threadArgs->fileContent->begin(); i != threadArgs->fileContent->end(); ++i){
			std::cout << *i << '\n';
			SORT_Node a;
			a.key = *i;
			final_sorted.push_back(a);
		}
		std::cout << "end" << '\n';
		*/
		pthread_t thread;
		pthread_create(&thread, 0, sort_map, (void *)threadArgs);
		threads.push_back(thread);
		
		all_maps.push_back(*(threadArgs));
	}
	
		/*wait for each map thread to finish*/
	for (vector<pthread_t>::iterator t_it = threads.begin(); t_it < threads.end(); t_it++){
		pthread_join(*t_it, 0);
	}
	
		
	reduce_maps->all_maps = &all_maps;
	
	/*one reduce thread is called to get the first element of each map and add it to the final sorted list*/
	pthread_t thread;
	pthread_create(&thread, 0, sort_reduce, (void *)reduce_maps);
	pthread_join(thread, 0);
	
	
	/*output to user chosen file name*/
	ofstream outfile;
	outfile.open(args.outfile.c_str());
	if (!outfile.is_open()) {
		cerr << "ERROR: Unable to open output file, " << args.outfile <<"."<<endl;
		return 1;
	}
	for (vector<SORT_Node>::iterator wc_it = final_sorted.begin(); wc_it < final_sorted.end(); wc_it++)
		outfile << wc_it->key << endl;
	outfile.close();
	
	return 0;
	
}

/*Sort Map*/
void *sort_map(void *arguments) 
{
	SORT_MapStruct *args = (SORT_MapStruct *)arguments;
	sort(args->fileContent->begin(), args->fileContent->end(), SORT_Node::compareTo);
	return 0;
}



/*Sort Reduce*/
void *sort_reduce(void *arguments){
	SORT_ReduceStruct *args = (SORT_ReduceStruct *)arguments;
	while((int)(args->all_maps->size()) != 0){
		/*go through all_maps elements and if any don't equal zero then the boolean will go to false and the loop will not exit*/
		bool zero = true;
		 for (std::vector<struct SORT_MapStruct>::iterator it = args->all_maps->begin() ; it != args->all_maps->end(); ++it){
			struct SORT_MapStruct temp = *it;
			vector<int> *tempints = it->fileContent;
			int l = tempints->size();
			if(l != 0){
				/*std::cout << "entered zero break" << '\n';*/
				zero = false;
				break;
			}
		 }
		/*if the whole loop above went through and found all elements were zero, break, we're finished*/
		if(zero){
			break;
		}
		
		/*below looks for the smallest element in all vectors to add to the final SORT_Node vector*/
		
		/*this keeps the value of the iterator that had the lowest value so that it can be used to remove it after*/
		std::vector<struct SORT_MapStruct>::iterator lowest = args->all_maps->begin();
		int lowest_val = args->all_maps->begin()->fileContent->front();
		
		for (std::vector<struct SORT_MapStruct>::iterator it = args->all_maps->begin() ; it != args->all_maps->end(); ++it){
			struct SORT_MapStruct temp = *it;
			vector<int> *tempints = it->fileContent;
			int l = tempints->front();
			if(l < lowest_val){
				lowest_val = l;
				lowest = it;
			}
		}
		
	/*    std::cout << "current lowest: "<< lowest_val << '\n';*/
		/*remove the lowest element from all_maps*/
		vector<int> *tempints = lowest->fileContent;
		tempints->erase(tempints->begin());
		if(tempints->size() == 0){
			args->all_maps->erase(lowest);
		}
		/*add lowest element to beginning of final_sorted*/
		 SORT_Node newelem;
		 newelem.key = lowest_val;
		 args->final_sorted->push_back(newelem);
		 
	}
	return 0;
}


int wc_mapreduce(Args &args)
{
	vector<pthread_t> threads; /*holds all thread IDs*/
	vector<WC_Node> threadwork; /* holds all work of threads*/
	sem_init(&sem_mutex, 0, 1);
	
	/*create threads, assign threads a file, and send them off to work*/
	for (int i = 0; i < args.numMap; i++) {
		WC_MapStruct *threadArgs = new WC_MapStruct;
		char i_str[6];
		sprintf(i_str, "%d", i);
		string partition = args.infile + "." + i_str;
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
		
		/*if a partition file is empty, stop creation of threads*/
		if (threadArgs->fileContent->empty()) {
			delete threadArgs->fileContent;
			delete threadArgs;
			break;
		}

		threadArgs->threadwork = &threadwork;
		pthread_t thread;
		pthread_create(&thread, 0, wc_map, (void *)threadArgs);
		threads.push_back(thread);
	}
	
	/*wait for each map thread to finish*/
	for (vector<pthread_t>::iterator t_it = threads.begin(); t_it < threads.end(); t_it++)
		pthread_join(*t_it, 0);

	if (threadwork.empty()) {
		cout <<"ERROR: " <<args.infile <<" is empty." <<endl;
		return 1;
	}
	
	/*semaphore destroyed*/
	sem_destroy(&sem_mutex);
	
	/*sort all of key,value pairs created by map threads*/
	sort(threadwork.begin(), threadwork.end(), WC_Node::compareTo);

	/*prep for reduce*/
	vector<WC_Node> *keyVector;
	pthread_t thread;
	pthread_cond_init(&WC_ReduceStruct::cv_master, 0);
	pthread_cond_init(&WC_ReduceStruct::cv_slave, 0);
	pthread_mutex_init(&WC_ReduceStruct::mtx_master, 0);
	WC_ReduceStruct reduceStructs[args.numReduce];

	/*create reduce threads and assign them key vectors*/
	pthread_mutex_lock(&WC_ReduceStruct::mtx_master);
	for (int i = 0; i < args.numReduce; i++) {
		keyVector = wc_getNextKeyVector(threadwork);
		if (keyVector == 0) {/*more workers than actual work...*/
			reduceStructs[i].die = true;
			continue;
		}

		reduceStructs[i].keyVector = keyVector;
		//reduceStructs[i].tid = i;
		//cerr << "init assigned next vector to "<<i<<" - "<<keyVector->front().key << "  "<< keyVector->size() << endl;
		pthread_create(&thread, 0, wc_reduce, (void *)(reduceStructs + i));
		pthread_detach(thread);
	}

	while (1) {
		pthread_cond_wait(&WC_ReduceStruct::cv_master, &WC_ReduceStruct::mtx_master);

		/*check if work is to be assigned to a slave*/
		int numDead = 0;
		for (int i = 0; i < args.numReduce; i++) {
			if (!reduceStructs[i].die && reduceStructs[i].done) {
				if ((keyVector = wc_getNextKeyVector(threadwork)) != 0) {
					reduceStructs[i].keyVector = keyVector;
					//cerr << "assigned next vector to "<<i<<" - "<<keyVector->front().key << "  "<< keyVector->size() << endl;
					reduceStructs[i].done = false;
				} else {
					reduceStructs[i].die = true;
					numDead++;
				}
				pthread_cond_signal(&WC_ReduceStruct::cv_slave); /*signal slave to work or die*/
			} else if (reduceStructs[i].die)
				numDead++;
		}

		if (numDead == args.numReduce) /*all slaves are dead, we are done*/
			break; 
	}
	pthread_mutex_unlock(&WC_ReduceStruct::mtx_master);
	pthread_cond_destroy(&WC_ReduceStruct::cv_master);
	pthread_cond_destroy(&WC_ReduceStruct::cv_slave);
	pthread_mutex_destroy(&WC_ReduceStruct::mtx_master);

	/*sort all of key,value pairs created by reduce threads*/
	sort(WC_ReduceStruct::reduceNodes.begin(), WC_ReduceStruct::reduceNodes.end(), WC_Node::compareTo);


	/*write results to output file*/
	//int count =0;
	ofstream outfile;
	outfile.open(args.outfile.c_str());
	if (!outfile.is_open()) {
		cerr << "ERROR: Unable to open output file, " << args.outfile <<"."<<endl;
		return 1;
	}
	for (vector<WC_Node>::iterator wc_it = WC_ReduceStruct::reduceNodes.begin(); wc_it < WC_ReduceStruct::reduceNodes.end(); wc_it++)
		outfile << wc_it->key << " " << wc_it->count << endl;
	outfile.close();
	//cout << count<<endl;

	return 0;
}


/*Given a sorted vector full of key,value nodes from map workers, 
 * retrieve a single key group by removing it from original vector
 * and returning the subvector*/
vector<WC_Node>* wc_getNextKeyVector(vector<WC_Node> &threadwork)
{
	if (threadwork.size() == 0)
		return 0;

	string curKey = threadwork[0].key;
	vector<WC_Node>::iterator node_it;
	for (node_it = threadwork.begin()+1; node_it < threadwork.end(); node_it++)
		if (node_it->key != curKey)
			break;
	vector<WC_Node> *curKeyVector = new vector<WC_Node>(threadwork.begin(), node_it);
	threadwork.erase(threadwork.begin(), node_it);
	//cerr << "getting next vector - "<<curKeyVector->front().key << "  "<< curKeyVector->size() << endl;
	return curKeyVector;
}


/*WordCount Map*/
void *wc_map(void *arguments) 
{
	WC_MapStruct *args = (WC_MapStruct *)arguments;

	/*get each word of file and push it to threadwork vector*/
	for (list<string>::iterator line_it = args->fileContent->begin(); line_it != args->fileContent->end(); line_it++) {
		transform(line_it->begin(), line_it->end(), line_it->begin(), ::tolower);
		vector<string> wordVector; /*vector containing word tokens in line*/

		/*tokenize the line into words*/
		size_t prev = 0, pos;
		while ((pos = line_it->find_first_not_of("abcdefghijklmnopqrstuvwxyz", prev)) != string::npos)
		{
			if (pos > prev)
				wordVector.push_back(line_it->substr(prev, pos-prev));
			prev = pos+1;
		}
		if (prev < line_it->length())
			wordVector.push_back(line_it->substr(prev, string::npos));

		/*Go through word tokens, check if not empty, and push to vector for this threads work*/
		for (vector<string>::iterator word_it = wordVector.begin(); word_it < wordVector.end(); word_it++) {
			if (*word_it != "") {
				sem_wait(&sem_mutex);
				args->threadwork->push_back(WC_Node(*word_it, 1));
				sem_post(&sem_mutex);
			}
		}
	}
	
	delete args->fileContent;
	delete args;
	return 0;
}

/*WordCount Reduce*/
void *wc_reduce(void *arguments)
{
	WC_ReduceStruct *args = (WC_ReduceStruct *)arguments;
	while (!args->die) {
		/*count up total words*/
		int wcount = 0;
		for (vector<WC_Node>::iterator key_it = args->keyVector->begin(); key_it < args->keyVector->end(); key_it++)
			wcount += key_it->count;
		WC_Node node(args->keyVector->front().key, wcount);
		delete args->keyVector;

		/*push work to global reduce work vector, flag done, and signal master for more work*/
		pthread_mutex_lock(&WC_ReduceStruct::mtx_master);
		//cerr << "thread finished "<<args->tid<<" " << node.key << " " << node.count << endl<< endl;
		args->done = true;
		WC_ReduceStruct::reduceNodes.push_back(node);
		pthread_cond_signal(&WC_ReduceStruct::cv_master);
		pthread_cond_wait(&WC_ReduceStruct::cv_slave, &WC_ReduceStruct::mtx_master);
		pthread_mutex_unlock(&WC_ReduceStruct::mtx_master);
	}
	return 0;
}
