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
	CacheController singlecore = CacheController(id, config, tracefile);
	//singlecore.runTracefile();
}

/*
	This function accepts a configuration file and a trace file on the command line.
	The code then initializes a cache simulator and reads the requested trace file(s).
*/
int main(int argc, char* argv[]) {
	CacheInfo config;

    std::vector<CacheController> cacheLevels;

	if (argc < 3) {
		cerr << "You need two command line arguments. You should provide a configuration file and a trace file." << endl;
		return 1;
	}

	string tracefile(argv[2]);
	
    // determine how many cache levels the system is using
	//unsigned int numCacheLevels;

	// read the configuration file
	cout << "Reading config file: " << argv[1] << endl;
	ifstream infile(argv[1]);
	unsigned int tmp;
	infile >> config.numCacheLevels;
    infile >> config.memoryAccessCycles;
    
    for (unsigned int i = 0; i < config.numCacheLevels; i++) {

	infile >> config.numberSets;
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

    //MultiCache caches = cacheLevels;

    for (unsigned int u = 0; u < cacheLevels.size(); u++) {

        if (u == cacheLevels.size()) {

            cacheLevels[u].nextCache = NULL;
        
        }

        else { 
            cacheLevels[u].nextCache = &(cacheLevels[u+1]);
        }
    }

	infile.close();

    srand (time(NULL)); // Seed for random block access
	
	// Examples of how you can access the configuration file information
	cout << "System has " << config.numCacheLevels << " cache(s)." << endl;
	cout << cacheLevels[0].ci.numberSets << " sets with " << cacheLevels[0].ci.blockSize << " bytes in each block. N = " << config.associativity << endl;

	if (config.rp == ReplacementPolicy::Random)
		cout << "Using random replacement protocol" << endl;
	else
		cout << "Using LRU protocol" << endl;
	
	if (config.wp == WritePolicy::WriteThrough)
		cout << "Using write-through policy" << endl;
	else
		cout << "Using write-back policy" << endl;

	// start the cache operation...

    cacheLevels[0].runTracefile();

	return 0;
}
