Tested on Linux Debian 7 Wheezy.
Compilation command:

	g++ -std=c++11 -g -pthread -o StrongestPath StrongestPath.cc


Execution command:

	./StrongestPath graphfilename source_targetfilename outputfilename

	Example:
	./StrongestPath graph4.txt source_target4.txt output4.txt


The program asks you first about the edges' representation in the graph file, if they are represented by one ligne or two lignes in the file.


The program requires the C++11 version of the standard for the programming language C++ ("-std=c++11" must be added in the compilation command).


This program uses dictionaries (represented as multimap structure in the algorithm) in order to speed up the search processing. Two dictionaries has been used, one to represent all the nodes and their edges in the graph, and another to represent the strength between the source and every parsed node.


The program also uses multithread to increase the execution speed, where each destination is associated to a thread, and those threads are executed parallel.
