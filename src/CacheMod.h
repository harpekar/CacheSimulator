/*
	Karen Harper
*/

#ifndef _CACHEMOD_H_
#define _CACHEMOD_H_

#include <string>
#include <condition_variable>

#include "CacheController.h"
#include "CacheStuff.h"
#include "BlockEntry.h"

void writeToCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response);
void readFromCache(std::list<BlockEntry>& CacheEntry, unsigned long int tag, CacheInfo config, CacheResponse* response);

#endif //CACHEMOD
