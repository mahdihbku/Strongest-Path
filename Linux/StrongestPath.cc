#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <map>
#include <utility>
#include <fstream>
#include <vector>
#include <sstream>
#include <thread>
#include <iostream>
#include <string>

using namespace std;

static const int num_threads = 8;		// This constant contains the maximum number of threads that can be used

struct neighbor {				//This structure represents the neighboring information on a node
	string n2;
	double w1;
};

struct nodeTreatmentInfo {		//This structure contains the best weight and the containing path number in the list of all possible paths for a given node
	double weight;
	int pathNumber;
};

string getLastNodeInPath(const std::vector<string>& path)		//This function returns the last node in the path "path"
{
	int lastPosition = path.size();
	return path[lastPosition - 1];
}

bool nodeExistsInPath(vector<string> path, string node)		//This function returns true if the node "node" exists in the path "path"
{
	bool found = false;
	int i = 0;
	while (i < path.size())
	{
		if (path[i] == node)
			found = true;
		i++;
	}
	return found;
}

bool nodeDeservesNewPath(const multimap<string, neighbor>& graph, const vector<vector<string> >& pathsList, const multimap<string, nodeTreatmentInfo>& allTreatedNodes, 
	double newWeight, int pathsCounter, string node)	// This function returns true if the node "node" deserves to be added at the end of a new path and added possible paths to the destination according to the its new weight "newWeight"
{	
	bool deserves = true;
	if (nodeExistsInPath(pathsList[pathsCounter], node))	// If the node is already in this path, then it shouldn't be added to a new path, in order to prevent loops in the graph
	{
		deserves = false;
	}
	else   //If the old weight from the source to this node (saved already in the treated nodes list "allTreatedNodes") is bigger than the new weight "newWeight", then it shouldn't be added to a new path
	{
		multimap<string, nodeTreatmentInfo>::const_iterator i = allTreatedNodes.find(node);
		if (i != allTreatedNodes.end() && i->second.weight > newWeight)
			deserves = false;
	}
	return deserves;
}

void writeShortestPathToOutputFile(string outputFile, const vector<vector<string> >& pathsList, const vector<double>& pathsWeightsList, const multimap<string, nodeTreatmentInfo>& allTreatedNodes, string destination)	
// This procedure gets the strongest path to the destination "destination" from all the possible paths list "pathsList", and writes the output the output file "outputFile"
{
	int id_bestPath = -1;
	double bestWeight = 0;
	for (int i = 0; i < pathsList.size(); i++)
	{
		if (nodeExistsInPath(pathsList[i], destination) && pathsWeightsList[i] > bestWeight)
		{
			id_bestPath = i;
			bestWeight = pathsWeightsList[i];
		}
	}
	if (id_bestPath != -1 && bestWeight != 0)
	{
		ofstream outFile;
		outFile.open(outputFile, ios::app);
		outFile << pathsList[id_bestPath][0] << " " << destination << " " << bestWeight << ":  " << pathsList[id_bestPath][0] << " ";
		for (int j = 1; j < pathsList[id_bestPath].size(); j++)
		{
			string nodeInPath = pathsList[id_bestPath][j];
			multimap<string, nodeTreatmentInfo>::const_iterator it = allTreatedNodes.find(nodeInPath);
			outFile << it->second.weight << " " << nodeInPath << " ";
		}
		outFile << endl;
		outFile.close();
	}
	else
	{
		ofstream outFile;
		outFile.open(outputFile, ios::app);
		outFile << "source:" << pathsList[0][0] << " target:" << destination << " no existent path "<< endl;
		outFile.close();
	}
}

void addNodeToTreatedNodesList(string node, int pathNumber, double newWeight, multimap<string, nodeTreatmentInfo> &allTreatedNodes)
//This procedure adds the node "node" to the treated nodes list according to its weight
{
	multimap<string, nodeTreatmentInfo>::iterator i = allTreatedNodes.find(node);
	if ( i == allTreatedNodes.end())	// If the node "node" hasn't been add to the treated nodes before, then the procedure adds it in treated nodes list
	{
		nodeTreatmentInfo treatementinfo;
		treatementinfo.pathNumber = pathNumber;
		treatementinfo.weight = newWeight;
		allTreatedNodes.insert(pair < string, nodeTreatmentInfo>(node, treatementinfo));
	}
	else   // If the node is already in the treated nodes list, and its old weight is weaker than its new weight "newWeight", then the node's value must be updated in the treated nodes list
	{
		if (i->second.weight < newWeight)
		{
			i->second.weight = newWeight;
			i->second.pathNumber = pathNumber;
		}
	}
}

void getStrongestPath(multimap<string, neighbor>& graph, string source, string destination, string outputFile)	
// This procedure is the main in our program, it allow getting the strongest path from the source "source" to the destination "destination" and writes the results in the output file "output"
{
	std::cout << "_________for destination: " << destination << endl;
	vector<vector<string> > possiblePaths;					// This variable contains the list of all treated paths proceeding from the source "source"
	vector<double> possiblePathsWeight;						// This variable contains the list of all paths' weight (keeping the same order as the possible paths list "possiblePaths")
	multimap<string, nodeTreatmentInfo> allTreatedNodes;	// This list (dictionary) contains the best strongest weight from the source to each processed node
	int possiblePathCounter = 0;							// This counter is used to parse all the paths in the possible paths list

	vector<string> path;
	double current_w = 1;
	double strongestPathWeight = 0;							// This variable contains the value of the weight of the strongest path treated

	// Adding the source node as the first path to be processed in the paths list ####################
	string current_node = source;
	nodeTreatmentInfo sourceInfo;
	sourceInfo.weight = 1;
	sourceInfo.pathNumber = 0;
	allTreatedNodes.insert(pair < string, nodeTreatmentInfo>(source, sourceInfo));
	string next_node;
	path.push_back(current_node);
	possiblePaths.push_back(path);
	possiblePathsWeight.push_back(1);
	//#################################################################################################

	do      // While not all paths in possiblePaths are processed then:
	{
		bool moreNodesToCheck = true;		// This variable is false if all the neighbors of the node "current_node" has been processed
		
		/*We move to another path in the list if:
		1. The last node added to the current path is the destination "destination"
		2. The current path becomes weaker than the best treated ever
		3. There is no more nodes to be treated from this path*/
		while (current_node != destination && current_w > strongestPathWeight && moreNodesToCheck)
		{
			vector<string> pathCopy = path;
			pair<multimap<string, neighbor>::const_iterator, multimap<string, neighbor>::const_iterator> keyRange = graph.equal_range(current_node);	// "KeyRange" contains the list of neighbors of the node "current_node" from the graph dictionary "graph"
			multimap<string, neighbor>::const_iterator s_it;
			s_it = keyRange.first;
			bool toBeAddedToCurrentPath = true;		// The variable "toBeAddedToCurrentPath" is used to store the first new valid neighbor to the current in the current path. All other valid neighbors must be added in new paths
			current_w = possiblePathsWeight[possiblePathCounter];
			next_node = "";
			bool newNodeInThisPath = false;
			while (s_it != keyRange.second)
			{
				neighbor N2 = (*s_it).second;
				string n2 = N2.n2;
				double w1 = N2.w1;

				double newWeight = w1 * current_w;
				if (nodeDeservesNewPath(graph, possiblePaths, allTreatedNodes, newWeight, possiblePathCounter, n2))		// If the new neighbor "n2" is checked by the function "nodeDeservesNewPath" to check if it deserves to be added to a possible path.
				{
					if (toBeAddedToCurrentPath)		// If the node "n2" must be added to the current path, then:
					{
						// 1. Add the node to the end of the current path "path"
						path.push_back(n2);
						// 2. Update/insert its weight in the treated nodes list using the function "addNodeToTreatedNodesList"
						addNodeToTreatedNodesList(n2, possiblePathCounter, possiblePathsWeight[possiblePathCounter] * w1, allTreatedNodes);
						next_node = n2;
						// 3. Update the path in the possible paths list
						possiblePaths[possiblePathCounter] = path;
						// 4. Update the weight of the current path in the weights of paths list
						possiblePathsWeight[possiblePathCounter] = possiblePathsWeight[possiblePathCounter] * w1;
						toBeAddedToCurrentPath = false;
						newNodeInThisPath = true;
					}
					else   // If the node "n2" must be added to a new path, then:
					{
						vector<string> newPossiblePath = pathCopy;
						// 1. Add the node to the end of the current path, as a new possible path
						newPossiblePath.push_back(n2);
						// 2. Update/insert its weight in the treated nodes list using the function "addNodeToTreatedNodesList"
						addNodeToTreatedNodesList(n2, possiblePathsWeight.size() + 1, current_w * w1, allTreatedNodes);
						// 3. add the path in the possible paths list "possiblePaths"
						possiblePaths.push_back(newPossiblePath);
						// 4. Update the weight of the new path in the weights of paths list "possiblePathsWeight"
						possiblePathsWeight.push_back(current_w * w1);
						if (n2 == destination && strongestPathWeight < current_w * w1) //If the new node "n2" is the destination, then update the value of the weight for the strongest path "strongestPathWeight" if the current path is stronger
						{
							strongestPathWeight = current_w * w1;
							std::cout << "new best path from " << source << " to " << destination << ", new weight: " << strongestPathWeight << endl;
							std::cout << "      new best path: ";
							for (int k = 0; k < possiblePaths[possiblePathCounter].size() - 1; k++)
							{
								std::cout << possiblePaths[possiblePathCounter][k] << ", ";
							}
							std::cout << destination << ", ";
							std::cout << endl;
						}
						double treatedPathsPercentage = (double)possiblePathCounter/(double)possiblePaths.size()*100;
						int remainingPaths = possiblePaths.size() - possiblePathCounter;
						std::cout << "Destintion: " << destination << " , total possible paths: " << possiblePaths.size() << " , treated: " << treatedPathsPercentage << "%." << endl;
					}
				}
				++s_it;

			}
			current_w = possiblePathsWeight[possiblePathCounter];
			if (newNodeInThisPath)
				current_node = next_node;
			else
				moreNodesToCheck = false;

		}
		if (possiblePathsWeight[possiblePathCounter] > strongestPathWeight && current_node == destination)  //If the current node "current_node" is the destination, then update the value of the weight for the strongest path "strongestPathWeight" if the current path is stronger
		{
			strongestPathWeight = possiblePathsWeight[possiblePathCounter];
			std::cout << "new best path from " << source << " to " << destination << ", new weight: " << strongestPathWeight << endl;
			std::cout << "      new best path: ";
			for (int k = 0; k < possiblePaths[possiblePathCounter].size(); k++)
			{
				std::cout << possiblePaths[possiblePathCounter][k] << ", ";
			}
			std::cout << endl;
		}
		possiblePathCounter++;		// Move to the next path in the paths list "possiblePaths"
		if (possiblePathCounter < possiblePaths.size())
		{
			path = possiblePaths[possiblePathCounter];
			current_w = possiblePathsWeight[possiblePathCounter];
			current_node = getLastNodeInPath(path);
		}
	} while (possiblePathCounter<possiblePaths.size()); 		
	writeShortestPathToOutputFile(outputFile, possiblePaths, possiblePathsWeight, allTreatedNodes, destination);		// Write the output to the output file "outputFile"
}

int main(int argc, char* argv[])
{	
	string graphFile = "";
	string outputFile = "";
	string source_targetFile = "";

	if (argc == 4)
	{
		graphFile=argv[1];
		source_targetFile=argv[2];
		outputFile=argv[3];
		ifstream infile(graphFile);
		if (infile == NULL)
		{
			std::cout << "Problem reading graph file!!!\n\n";
			return 0;
		}
		ifstream infile2(source_targetFile);
		if (infile2 == NULL)
		{
			std::cout << "Problem reading source_target file!!!\n\n";
			return 0;
		}
		string n1, n2, w1;
		multimap<string, neighbor> graph;		// "graph" is a variable that contains the list of all edges and their weights as a dictionary to ease the research function
		ofstream outFile;
		outFile.open(outputFile, ios::out);
		outFile.close();

		std::cout << "How are your edges (a-b) represented in the graph file ?\n1.bidirectional inputs (a b,b a)\n2.unidirectional inputs(a b)"<<endl;
		int reply = -1;
		cin >> reply;

		// Filling the graph "graph" from the input files "graphFile"################################################
		do
		{
			if (reply = 2)
			{
				while (infile >> n1 >> n2 >> w1)
				{
					if (!n1.empty() && !n2.empty() && !w1.empty())
					{
						neighbor N2;
						N2.n2 = n2;
						double weight1 = stod(w1);
						N2.w1 = weight1;
						if (weight1 >= 0 && weight1 < 1)
						{
							graph.insert(pair <string, neighbor>(n1, N2));
							neighbor N3;
							N3.n2 = n1;
							N3.w1 = weight1;					
							graph.insert(pair <string, neighbor>(n2, N3));
						}

					}
				}
			}
			if (reply = 1)
			{
				while (infile >> n1 >> n2 >> w1)
				{
					if (!n1.empty() && !n2.empty() && !w1.empty())
					{
						neighbor N2;
						N2.n2 = n2;
						double weight2 = stod(w1);
						N2.w1 = weight2;
						if (weight2 >= 0 && weight2 < 1)							
							graph.insert(pair <string, neighbor>(n1, N2));
					}
				}
			}
		} while (reply != 1 && reply != 2);
		// #############################################################################################################

		// Filling the destination list "destinations" from the input files "source_targetFile"#########################

		string source, d, blank;
		vector<string> destinations;
		infile2 >> source;
		std::cout << "source: " << source << endl;
		while (infile2 >> d)
		{
			destinations.push_back(d);
		}
		// #############################################################################################################

		// Starting the search of the strongest path parallel for each destination######################################
		int i = 0;
		vector<thread> tt;
		string destination;
		while (i<destinations.size())
		{
			destination = destinations[i];
			//getStrongestPath(graph, source, destination, outputFile);
			tt.push_back(thread(getStrongestPath, std::ref(graph), source, destination, outputFile));
			i++;
		}
		for (auto& th : tt) th.join();
		// #############################################################################################################
	}
	else
	{
		cout << "problem with files names from arguments" << endl;
	}
	return 0;
}
