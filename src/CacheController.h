/*
	Karen Harper
	Oregon State University
	Spring Term 2021
*/

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include "CacheStuff.h"
#include "CacheMod.h"
#include "BlockEntry.h"
#include <string>
#include <fstream>
#include <vector>

class CacheController {
	private:
		struct AddressInfo {
			unsigned long int tag;
			unsigned int setIndex;
		};
		
        
        unsigned int globalCycles;
		unsigned int globalHits;
		unsigned int globalMisses;
		unsigned int globalEvictions;

        unsigned int readNum;
        unsigned int writeNum;

		std::string inputFile, outputFile;

        //Add vector of Cache Blocks for cache system
        std::vector<std::list<BlockEntry>> programCache;
        //std::list<CacheBlock> 

		CacheInfo ci;

		// function to allow read or write access to the cache
		void cacheAccess(CacheResponse*, bool, unsigned long int, int);
		// function that can compute the index and tag matching a specific address
		AddressInfo getAddressInfo(unsigned long int);
		// function to add entry into output file
		void logEntry(std::ofstream&, CacheResponse*);
		

	public:
		CacheController(CacheInfo, std::string);
		void runTracefile();
};

#endif //CACHECONTROLLER
