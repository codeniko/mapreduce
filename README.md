mapreduce
=========

cs416 mapreduce
nikolay feldman, janelle barcia

instructions for running this program: upon calling 'make', this program should run with the following command line structure: mapred -a [wordcount, sort] -i [procs, threads] -m num_maps -r num_reduces infile outfile

description of the framework: after error checking the user inputs, fork() and execlp() are called on split.sh to split the input file into map number of files. after splitting, main calls sort or wordcount depending on user input.
When wordcount is called, 
When sort is called, a function called 'sort_mapreduce' takes in the user arguments (in the form of an Arg struct contained within the header). A loop first to put splited files put each line into a SORT_MapStruct struct and then a thread is created to pass this struct to the sort_map method, this runs the number of map times denoted by the user. In sort_map called with each thread, the list of lines is sorted. Upon the thread joining back to the sort_mapreduce method, this sorted list is stored in a vecture structure called all_maps in SORT_ReduceStruct, that will be passed to the sort_reduce (which is run with only one thread). sort_reduce gets all of the individually sorted maps stored in the all_maps vector, it reduces them by finding the lowest of the first indexes of all the maps. The lowest of all the maps is put into a final_sorted vector of SORT_Nodes and is then removed for the original SORT_ReduceStruct. This continues untill all maps in all_maps are empty. The final_sorted vector is written to the output file.

testcases: For sort multiple tests were conducted using the provided bash script (randomnums.sh) to create data sets to sort. For word count, we tested using multiple lengthy books found on Project Gutenberg.

difficulties/problems faced: One of the main problems faced was thinking of ways to reduce sort in an efficieny manner. We soon realized that if we had more time, we would change sort from ascending to descending order so that when each of the first elements is removed, all elements don't have to shift forward. This way we would use pop_back() instead of being()/front(), and would overall be more efficient.