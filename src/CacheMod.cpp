/*

   Karen Harper

*/

#include <iostream>
#include <list>
#include <math.h>

#include "CacheController.h"
#include "CacheSimulator.h"
#include "CacheMod.h"

void writeToCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response){
    
int numAddnAccess = int((ceil(config.blockSize - 8)/8)); 

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
            response->cycles += config.memoryAccessCycles + numAddnAccess;
        }
        else {
            CacheEntry.front().dirtyBit = true;

            std::cout << CacheEntry.front().dirtyBit << std::endl;
        }

        return;
    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

BlockEntry requBlock {false,true,tag};

if (config.wp == WritePolicy::WriteThrough)
    response->cycles += config.memoryAccessCycles + numAddnAccess;

//If this line of the cache is full, it must be an eviction
 
if (CacheEntry.size() == config.associativity) {

    if ((config.wp == WritePolicy::WriteBack) && (CacheEntry.back().dirtyBit == true)) {
        std::cout << "Writing back" << std::endl; 
        response->cycles += config.memoryAccessCycles + numAddnAccess;
    }

    if (config.rp == ReplacementPolicy::LRU) {
        CacheEntry.pop_back();
    }
    
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

CacheEntry.front().dirtyBit = true;

response->misses += 1;

std::cout << "A Miss" << std::endl;

//Must update main memory after write is completed
//if (config.wp == WritePolicy::WriteThrough) { 


response->cycles += (config.memoryAccessCycles + config.cacheAccessCycles + numAddnAccess);

//}

return;

}


void readFromCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response) {

int numAddnAccess = int((ceil(config.blockSize - 8)/8)); 

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

    if ((config.wp == WritePolicy::WriteBack) && (CacheEntry.back().dirtyBit == true)) 
        response->cycles += config.memoryAccessCycles + numAddnAccess;

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

//Miss

BlockEntry requBlock {false,true,tag};

//Determine if the main memory will have to be accessed more than once 

response->cycles += config.memoryAccessCycles + numAddnAccess;

CacheEntry.push_front(requBlock);
response->misses += 1;

std::cout << "A miss" << std::endl;


return;

}

