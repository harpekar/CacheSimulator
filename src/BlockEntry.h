/*
	Karen Harper
*/
#ifndef _BLOCKENTRY_H_
#define _BLOCKENTRY_H_

#include <utility>
#include <list>

//This file stores the cache configuration for each cache level.

//Valid Bit, Dirty Bit, Tag Bits

//typedef std::tuple <bool, bool, unsigned long int> CacheBlock;

class BlockEntry {
    public:    
        bool dirtyBit;
        bool validBit;
        unsigned long int tagBits;
};


#endif //BLOCKENTRY
