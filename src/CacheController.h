/*
	Karen Harper
	Oregon State University
	Spring Term 2021
*/

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include "CacheStuff.h"
//#include "CacheMod.h"
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
		
        
        unsigned int cycles;
		unsigned int hits;
		unsigned int misses;
		unsigned int evictions;

        unsigned int globalRead;
        unsigned int globalWrite;

		std::string inputFile, outputFile;

        //Add vector of Cache Blocks for cache system
        //std::list<CacheBlock> 

        //std::vector<CacheController> *caches;

        unsigned int writeCycles();

		// function to allow read or write access to the cache
		void cacheAccess(CacheResponse*, bool, unsigned long int, int);

        void writeToCache(CacheResponse*, unsigned long int, unsigned long int, int);
        void readFromCache(CacheResponse*, unsigned long int, unsigned long int, int);

        void evictTo(unsigned long int, unsigned long int);
        void addTo(unsigned long int, unsigned long int);

		// function that can compute the index and tag matching a specific address
		AddressInfo getAddressInfo(unsigned long int);
		// function to add entry into output file
		void logEntry(std::ofstream&, CacheResponse*);
		

	public:
		
        std::vector<std::list<BlockEntry>> programCache;

        CacheController(int, CacheInfo, std::string);
		void runTracefile();

        unsigned int level;
		CacheInfo ci;
        CacheController *nextCache;
};

#endif //CACHECONTROLLER
