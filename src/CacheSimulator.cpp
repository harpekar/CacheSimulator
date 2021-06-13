/*
	Karen Harper
	Oregon State University
	Spring Term 2021
*/

#include "CacheSimulator.h"
#include "CacheStuff.h"
#include "CacheController.h"

#include <iostream>
#include <fstream>

#include <stdio.h>      /* printf, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

#include <list>

using namespace std;

/*
	This function creates the cache and starts the simulator.
	Accepts core ID number, configuration info, and the name of the tracefile to read.
*/
void initializeCache(int id, CacheInfo config, string tracefile) {
	//CacheController singlecore = CacheController(id, config, tracefile);
	//singlecore.runTracefile();
}

/*
	This function accepts a configuration file and a trace file on the command line.
	The code then initializes a cache simulator and reads the requested trace file(s).
*/
int main(int argc, char* argv[]) {

    std::vector<CacheController> cacheLevels;

	if (argc < 3) {
		cerr << "You need two command line arguments. You should provide a configuration file and a trace file." << endl;
		return 1;
	}

	string tracefile(argv[2]);
	
    // determine how many cache levels the system is using

	// read the configuration file
	cout << "Reading config file: " << argv[1] << endl;
	ifstream infile(argv[1]);
	unsigned int tmp;
	
    unsigned int numCacheLevels;
    unsigned int memoryAccessCycles;

    infile >> numCacheLevels;
    infile >> memoryAccessCycles;
    
    for (unsigned int i = 0; i < numCacheLevels; i++) {
    
	CacheInfo config;

    config.numCacheLevels = numCacheLevels;
    config.memoryAccessCycles = memoryAccessCycles;

	infile >> config.numberSets;

    //cout << config.numberSets << endl;

	infile >> config.blockSize;
	infile >> config.associativity;
	infile >> tmp;
	config.rp = static_cast<ReplacementPolicy>(tmp);
	infile >> tmp;
	config.wp = static_cast<WritePolicy>(tmp);
	infile >> config.cacheAccessCycles;

    CacheController cache = CacheController((i+1), config, tracefile);

    //cout << cache.ci.numberSets << endl;

    cacheLevels.push_back(cache);
    }

    for(unsigned int i = 0; i < (numCacheLevels - 1); i++) {

        cacheLevels[i].nextCache = &(cacheLevels[i+1]);    

    }

    cacheLevels.back().nextCache = NULL;

	infile.close();

    srand (time(NULL)); // Seed for random block access
	
	// Examples of how you can access the configuration file information
	cout << "System has " << numCacheLevels << " cache(s)." << endl;
	cout << cacheLevels[0].ci.numberSets << " sets with " << cacheLevels[0].ci.blockSize << " bytes in each block. N = " << cacheLevels[0].ci.associativity << endl;

	cout << cacheLevels[1].ci.numberSets << " sets with " << cacheLevels[1].ci.blockSize << " bytes in each block. N = " << cacheLevels[1].ci.associativity << endl;

    cout << cacheLevels[0].nextCache->programCache[0].size() << "it's 0" << endl;

    if (cacheLevels[0].ci.rp == ReplacementPolicy::Random)
		cout << "Using random replacement protocol" << endl;
	else
		cout << "Using LRU protocol" << endl;
	
	if (cacheLevels[0].ci.wp == WritePolicy::WriteThrough)
		cout << "Using write-through policy" << endl;
	else
		cout << "Using write-back policy" << endl;

	// start the cache operation...

    cacheLevels[0].runTracefile();

	return 0;
}
