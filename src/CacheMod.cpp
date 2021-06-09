/*

   Karen Harper

*/

#include <iostream>
#include <list>

#include "CacheController.h"
#include "CacheSimulator.h"
#include "CacheMod.h"

void writeToCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response){

//No matter what, we have to access the cache    
response->cycles += config.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = CacheEntry.begin(); block != CacheEntry.end(); block++) {

    if ((*block).tagBits == tag) { // If the write attempt is a hit 
        CacheEntry.push_front(*block);
        CacheEntry.erase(block);
        response->hits += 1;

        std::cout << "A Hit" << std::endl;

        if (config.wp == WritePolicy::WriteThrough) {
            response->cycles += config.memoryAccessCycles;
        }
        else {
            CacheEntry.front().dirtyBit = true;
        }

        return;
    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

BlockEntry requBlock {true,false,tag};

response->cycles += config.memoryAccessCycles;


//If this line of the cache is full, it must be an eviction
 
if (CacheEntry.size() == config.associativity) { 
    if (config.rp == ReplacementPolicy::LRU)
        CacheEntry.pop_back();
    
    else {
        int random = rand() % CacheEntry.size();
        block = CacheEntry.begin();
        advance(block, random);    
        CacheEntry.erase(block);

    }

    response->evictions += 1;

    std::cout << "An eviction" << std::endl;

}

CacheEntry.push_front(requBlock);
response->misses += 1;

std::cout << "A Miss" << std::endl;

//Must update main memory after write is completed
if (config.wp == WritePolicy::WriteThrough) { 

    response->cycles += (config.memoryAccessCycles + config.cacheAccessCycles);

}

return;

}


void readFromCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response) {

    
//No matter what, we have to access the cache    
response->cycles += config.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = CacheEntry.begin(); block != CacheEntry.end(); block++) {

    //std::cout << tag << std::endl;

    //std::cout << (*block).tagBits << std::endl;

    if ((*block).tagBits == tag) { // If the read attempt is a hit 
        CacheEntry.push_front(*block);
        CacheEntry.erase(block);
        response->hits += 1;

        std::cout << "A hit" << std::endl;


        return;
    }
}

//If this line of the cache is full, it must be an eviction
 
if (CacheEntry.size() == config.associativity) {
    if (config.rp == ReplacementPolicy::LRU)


        CacheEntry.pop_back();
    
    else {
        int random = rand() % CacheEntry.size();
        block = CacheEntry.begin();
        advance(block, random);    
        CacheEntry.erase(block);

    }

    response->evictions += 1;

    std::cout << "An eviction" << std::endl;

}

//If it is not a hit, it is a miss and possibly an eviction. 

BlockEntry requBlock {true,false,tag};

response->cycles += config.memoryAccessCycles;

std::cout << "size is " << CacheEntry.size() << std::endl;

CacheEntry.push_front(requBlock);
response->misses += 1;

std::cout << "A miss" << std::endl;


return;

}

