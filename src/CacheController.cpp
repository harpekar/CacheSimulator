/*
	Karen Harper
    Oregon State University
	Spring Term 2021
*/


#include <iostream>
#include <fstream>
#include <regex>
#include <cmath>
#include <list>

#include <bitset>

#include "CacheController.h"
#include "BlockEntry.h"
//#include "CacheMod.h"


using namespace std;


//CacheController constructor: Initializes counters for hit, miss, eviction, and cycle usage values, initializes cache, sets cache level

CacheController::CacheController(int id, CacheInfo ci, string tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = tracefile;
	this->outputFile = this->inputFile + ".out";
	// compute the other cache parameters

	this->ci.numByteOffsetBits = log2(ci.blockSize);

	this->ci.numSetIndexBits = log2(ci.numberSets);
	
    // initialize the counters
	this->hits = 0;
	this->misses = 0;
	this->evictions = 0;

    this->cycles = 0;

    this->level = id;


    //Initialize Cache
    std::vector<std::list<BlockEntry>> programCache;

    //Initialize each cache block at each address with the given associativity level
   
    //cout << ci.blockSize << endl;  

    for (unsigned int i = 0; i < ci.numberSets; i++) {

        std::list<BlockEntry> CacheEntry;

        //cout << "Size is " << CacheEntry.size() << endl;

        programCache.push_back(CacheEntry); 

    }

    this->programCache = programCache;
}

/*

    writeToCache: Function called to add a specified number of bytes to the CacheController at a specified address

    Inputs: Array of "response" objects, Integer representation of Address, number of Bytes to be written
    "isCont" specifies if the write is part of a sequence of writes or not 

*/

void CacheController::writeToCache(CacheResponse* responses, unsigned long int address, int numBytes, bool isCont){

AddressInfo ai = getAddressInfo(address); 

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8));

//No matter what, we have to access the cache  

responses[(level-1)].cycles += ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

//Test if the write attempt is a hit 
for (block = programCache.at(ai.setIndex).begin(); block != programCache.at(ai.setIndex).end(); block++) {

    AddressInfo blockAddr = getAddressInfo((*block).address); 

    if (blockAddr.tag == ai.tag) { 
        //If it is a hit, move the block to the front of the list at the specified point

        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);


        responses[(level-1)].hits += 1;

        if (ci.wp == WritePolicy::WriteThrough) {

            if (this->level < this->ci.numCacheLevels) {
                //responses[(level)].cycles += this->nextCache->ci.cacheAccessCycles; 
                this->nextCache->writeToCache(responses, address, numBytes, isCont);
            }

            else {
                responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;
            }
        }
        //If in WriteBack mode, set the block's Dirty Bit to True so the value can be written upon eviction

        else {
            programCache.at(ai.setIndex).front().dirtyBit = true;
        }
    return;

    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

//Initialize a new block to be written to the cache at specified index
BlockEntry requBlock {false,true,address};


//If this line of the cache is full, it must be an eviction 
if (programCache.at(ai.setIndex).size() == ci.associativity) {

    BlockEntry evicted = evict(ai.setIndex);

    //Upon eviction, write back to memory if specified and if value is last in line
    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (evicted.dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }

    //If not last in line, evict to next cache in sequence
    else {
        this->nextCache->evictTo(evicted);
    }
    //Keep tally of evictions 
    responses[(level-1)].evictions += 1;
}

//If a miss, Add new block to cache at index

programCache.at(ai.setIndex).push_front(requBlock);
programCache.at(ai.setIndex).front().dirtyBit = true;

responses[(level-1)].misses += 1;

if ((this->level) == 1) { 

    //If cache is L1, ensure cycles for probing and writing are counted
    responses[(level-1)].cycles += this->ci.cacheAccessCycles;
}

if ((this->level) < this->ci.numCacheLevels) {

    //If not last cache in line, assign appropriate hit and cycle usage values for the probe and response 

    if (this->ci.wp == WritePolicy::WriteThrough) {

        responses[level].cycles += this->nextCache->ci.cacheAccessCycles; 
        responses[level].hits += 1;

        //Write through to next caches, if specified     

        this->nextCache->writeToCache(responses, address, numBytes, isCont); 
    }
}


else {

    //If last in line, access main memory 

    responses[(level-1)].cycles += (ci.memoryAccessCycles + numAddnAccess);

    if (this->ci.wp == WritePolicy::WriteThrough) {

        //If in Write-through, memory must be probed and written to 
        if (!isCont) {
            responses[(level-1)].cycles += (ci.memoryAccessCycles + numAddnAccess);

        }
        else {
            //If memory already accessed, the "initial" access only takes 1 clock cycle
            responses[(level-1)].cycles += (numAddnAccess + 1);
        }
    }
}

return;

}

/*

    readFromCache: Function called to read a specified number of bytes to the CacheController at a specified address

    Inputs: Array of "response" objects, Integer representation of Address, number of Bytes to be read
    Outputs: None
*/

void CacheController::readFromCache(CacheResponse* responses, unsigned long int address, int numBytes) {

AddressInfo ai = getAddressInfo(address);

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

//Calculate if main memory will have to be accessed more than once
int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
responses[(level-1)].cycles += this->ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = programCache.at(ai.setIndex).begin(); block != programCache.at(ai.setIndex).end(); block++) {

    AddressInfo blockAddr = getAddressInfo((*block).address);

    //Test if block read is a Hit
    
    if (blockAddr.tag == ai.tag) { 
        //If it is, push the block to the front of the list and iterate counter 

        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);

        responses[(level-1)].hits += 1;
        return;
    }
}

//If this line of the cache is full, it must be an eviction
if (programCache.at(ai.setIndex).size() == this->ci.associativity) {

    BlockEntry evicted = evict(ai.setIndex);

    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (evicted.dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }

    //If not last in line, evict to next cache in sequence
    else {
        this->nextCache->evictTo(evicted);
    }
    responses[(level-1)].evictions += 1;
}

//If miss, create new block to be added to cache
BlockEntry requBlock {false,true,address};

programCache.at(ai.setIndex).push_front(requBlock);

responses[(level-1)].misses += 1;

//If not last in line, propagate read through cache line. If last in line, read from main memory 
if ((this->level) < (this->ci.numCacheLevels)) {
    this->nextCache->readFromCache(responses, address, numBytes); 

}
else {
    responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;
}

return;

}

/*
    addTo: Function that handles adding a block to a cache level without using clock cycles 
        (Such as during an eviction)
    Inputs: the block to be evicted 
    Outputs: None
 */

void CacheController::addTo(BlockEntry evicted) { 

    AddressInfo ai = getAddressInfo(evicted.address);
 
    BlockEntry block = evicted;

    block.dirtyBit = false;

    //If this index is full, evict a block to next cache level down 
    if (this->programCache.at(ai.setIndex).size() == ci.associativity) {

        cout << ci.numCacheLevels << endl;

        if ((level) < ci.numCacheLevels) {

            this->nextCache->evictTo(block);

        }
        
    }

    //Add block to cache level 

    this->programCache.at(ai.setIndex).push_front(block);

    return;

}

/*
    evictTo: Function that handles evicting from one cache level to the next

    Inputs: Block to be evicted to lower level, sent from higher level

    Outputs: None
*/


void CacheController::evictTo(BlockEntry evicted) {

    AddressInfo ai = getAddressInfo(evicted.address);

    std::list<BlockEntry>::iterator block;

    //Check if block already exists in cache

    for (block = this->programCache.at(ai.setIndex).begin(); block != this->programCache.at(ai.setIndex).end(); block++) {
        if ((*block).address == evicted.address) {

            return;
        }
    }


    //If evicted from last cache, block simply disappears
    if (level == ci.numCacheLevels) { 

        return;
    }

    if ((this->programCache.at(ai.setIndex).size() == ci.associativity)) {

        //If not last cache, but index is full, evict again to next cache level down

        BlockEntry removed = this->evict(ai.setIndex);
        this->nextCache->evictTo(removed); //Remove the correct block so new block can be added

    }

    //Add specified block to this cache level
    this->addTo(evicted);

    return;
}

/*

    evict: Processes the correct eviction for the cache based on Replacement Policy

    Inputs: Cache Index at which to process eviction
    Outputs: Block to be evicted (either least recently used or chosen randomly)
*/

BlockEntry CacheController::evict(int index) { 

    BlockEntry evicted;

    std::list<BlockEntry>::iterator block;
    
    if (ci.rp == ReplacementPolicy::LRU){

        evicted = programCache.at(index).back();

        programCache.at(index).pop_back();
    }

    else {

        //Compute random list index at which to remove block

        int random = rand() % programCache.at(index).size();
        block = programCache.at(index).begin();
        advance(block, random);   

        evicted = (*block);

        programCache.at(index).erase(block);
    }

    return evicted;

}

/*

    cacheAccess: Exclusively called for L1 cache, this function processes L1 operations and initializes recursion if necessary 

    Inputs: Array of CacheResponses to hold counters, Address of interest, number of Bytes to be read or written in the given operation
    Outputs: None

*/


void CacheController::cacheAccess(CacheResponse *responses, bool isWrite, unsigned long int address, int numBytes) {

    //Reset Counters

    for (unsigned int i = 0; i < 3; i++) {

        responses[i].hits = 0;
        responses[i].misses = 0;
        responses[i].evictions = 0;

        responses[i].cycles = 0;
    
    }

    //If the operation requests more bytes from a specified address than one block can handle, more than one operation is necessary

    double requOps = ceil((double)(numBytes + (address % ci.blockSize)) / ci.blockSize);

    for (int operations = 0; operations < int(requOps); operations++) {
 
    //Determine if the operation in question is a continuation of another
    // (Useful for computing final memory access values)    

        bool isCont = (operations != 0);    

        //If more than one operation, move address and numBytes counters accordingly 

        int adr = address + (ci.blockSize * operations);

        int remBytes = numBytes - (ci.blockSize * operations);

        if (isWrite) {
            writeToCache(responses, adr, remBytes, isCont);
        }

        else {
            readFromCache(responses, adr, remBytes);
        }

    }


    //Helper report statements to show operation result--only shows L1 but can easily add more
	/*if (responses[(level-1)].hits > 0)
		cout << "Operation at address " << std::hex << address << " caused " << responses[(level-1)].hits << " hit(s)." << std::dec << endl;
	if (responses[(level-1)].misses > 0)
		cout << "Operation at address " << std::hex << address << " caused " << responses[(level-1)].misses << " miss(es)." << std::dec << endl;

	cout << "-----------------------------------------" << endl;
    */
	
    return;
}

/*
    runTraceFile: Starts reading the tracefile and processing memory operations. Only called for L1.

    Inputs: None
    Outputs: None

*/
void CacheController::runTracefile() {
// process each input line
	string line;

	// define regular expressions that are used to locate commands
	regex commentPattern("==.*");
	regex instructionPattern("I .*");
	regex loadPattern(" (L )(.*)(,)([[:digit:]]+)$");
	regex storePattern(" (S )(.*)(,)([[:digit:]]+)$");
	regex modifyPattern(" (M )(.*)(,)([[:digit:]]+)$");

	// open the output file
	ofstream outfile(outputFile);
	// open the output file
	ifstream infile(inputFile);

    CacheResponse responses[3];

	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		} 
       
        //If a load, store, or modify, call these functions in order

        else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
            globalRead++;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(responses, false, address, stoi(match.str(4)));
			
            logEntry(outfile, responses);
			
		} 
        
        else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
            globalWrite++;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(responses, true, address, stoi(match.str(4)));

			logEntry(outfile, responses);
		} 
        
        else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			
            // first process the read operation
            globalRead++;
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(responses, false, address, stoi(match.str(4)));
			logEntry(outfile, responses);
			outfile << endl;

			// now process the write operation
            globalWrite++;
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(responses, true, address, stoi(match.str(4)));
			logEntry(outfile, responses);
		} 
        
        else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}


    //Iterate through Caches and report counter values 

    CacheController *cont = this;

    for (unsigned int i = 0; i < ci.numCacheLevels; i++) { 

	    // add the final cache statistics
	    outfile << "L" << (i+1) << " Cache: Hits:" << cont->hits << " Misses:" << cont->misses << " Evictions:" << cont->evictions << endl;

        cont = cont->nextCache;

    }


    //These counters are kept by the L1 cache, since they are global 

	outfile << "Cycles:" << this->cycles << " Reads:" << globalRead << " Writes:" << globalWrite << endl;
	infile.close();
    
    cout << "Closing file" << endl;

    outfile.close();
}
	

/*

    logEntry: Report the results of a memory access operation to the output file

    Inputs: output file object, array of CacheResponse objects

*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* responses) {

    int lineCycles = responses[0].cycles + responses[1].cycles + responses[2].cycles; 

    this->cycles += lineCycles;

    CacheController *cache = this;

    outfile << " " << (lineCycles) << " "; 
    for (unsigned int i = 0; i < ci.numCacheLevels; i++) {

        //If no operations were conducted, do not report in output file
        if ((responses[i].misses == 0) && (responses[i].hits == 0) && (responses[i].evictions == 0))
               break; 
	
        outfile << "L" << (i+1) << " ";

        //Keep track of correct number of hits, misses, evictions per operation

	    if (responses[i].misses > 0) {
		    outfile << "miss ";
            cache->misses += responses[i].misses;
        }

        if (responses[i].hits > 0) {
		    outfile << "hit ";
	        cache->hits += responses[i].hits;
        }

        if (responses[i].evictions > 0) {
		    outfile << "eviction ";
            cache->evictions += responses[i].evictions;
        }

        cache = cache->nextCache;
 
        }
}

/*
    getAddressInfo:Calculate the block index and tag for a specified address.
    Inputs: Address of interest
    Outputs: Simple AddressInfo object specifying tag and index values based on cache's block size and index values
 
 */
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (this->ci.numByteOffsetBits + this->ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << this->ci.numByteOffsetBits << this->ci.numSetIndexBits)) >> this->ci.numByteOffsetBits; 

	return ai;
}


