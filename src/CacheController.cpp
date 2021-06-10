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
    
    for (size_t i = 0; i < ci.numberSets; i++) {

        std::list <BlockEntry> CacheEntry; 
        programCache.push_back(CacheEntry); 

    }

    this->programCache = programCache;

}

unsigned int CacheController::writeCycles(){

    int cycles = ci.memoryAccessCycles;

    CacheController *cache = this;

    while (cache->nextCache != NULL) {

        cycles += cache->ci.cacheAccessCycles;
        
        cache = cache->nextCache;

    }


    //If main memory needs more than one read cycle

    int numAddnAccess = int((ceil(cache->ci.blockSize - 8)/8));

    cycles += numAddnAccess;

    return cycles;
}

void CacheController::writeToCache(CacheResponse* response, unsigned long int tag, unsigned long int index, int numBytes){
   
//std::list<BlockEntry> CacheEntry = programCache.at(index);

//int numOps = int(ceil(numBytes/ci.blockSize));

int numAddnAccess = int((ceil(ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
response->cycles += ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = programCache.at(index).begin(); block != programCache.at(index).end(); block++) {

    if ((*block).tagBits == tag) { // If the write attempt is a hit 
        programCache.at(index).push_front(*block);
        programCache.at(index).erase(block);
        response->hits += 1;

        std::cout << "A Hit" << std::endl;

        if (ci.wp == WritePolicy::WriteThrough) {
            response->cycles += ci.memoryAccessCycles + numAddnAccess;
        }
        else {
            programCache.at(index).front().dirtyBit = true;


        }

        if ((numBytes - signed(this->ci.blockSize)) > 0) {
            writeToCache(response, tag, (index+1), (numBytes-ci.blockSize));
        }

        //this->programCache.at(index) = CacheEntry;
        
        return;
    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

BlockEntry requBlock {false,true,tag};

if (ci.wp == WritePolicy::WriteThrough)
    response->cycles += ci.memoryAccessCycles + numAddnAccess;

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(index).size() == ci.associativity) {

    if ((ci.wp == WritePolicy::WriteBack) && (programCache.at(index).back().dirtyBit == true)) {
        std::cout << "Writing back" << std::endl; 
        response->cycles += ci.memoryAccessCycles + numAddnAccess;
    }

    if (ci.rp == ReplacementPolicy::LRU) {
        programCache.at(index).pop_back();
    }
    
    else {
        int random = rand() % programCache.at(index).size();
        block = programCache.at(index).begin();
        advance(block, random);    
        programCache.at(index).erase(block);

    }

    response->evictions += 1;

    std::cout << "An eviction" << std::endl;

}

programCache.at(index).push_front(requBlock);

programCache.at(index).front().dirtyBit = true;

//this->programCache.at(index) = CacheEntry;

response->misses += 1;

std::cout << "A Miss" << std::endl;

//Must update main memory after write is completed
//if (config.wp == WritePolicy::WriteThrough) { 

if ((numBytes - signed(this->ci.blockSize)) > 0) {
    readFromCache(response, tag, (index+1), (numBytes-ci.blockSize));
}


response->cycles += (ci.memoryAccessCycles + ci.cacheAccessCycles + numAddnAccess);

//}

return;

}


void CacheController::readFromCache(CacheResponse* response, unsigned long int tag, unsigned long int index, int numBytes) {

//std::list<BlockEntry> CacheEntry = this->programCache.at(index);

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
response->cycles += this->ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

cout << numBytes << endl;
cout << ci.blockSize << endl;

//cout << CacheEntry.size() << endl;

for (block = programCache.at(index).begin(); block != programCache.at(index).end(); block++) {


    if ((*block).tagBits == tag) { // If the read attempt is a hit 
        programCache.at(index).push_front(*block);
        programCache.at(index).erase(block);
        response->hits += 1;

        std::cout << "A hit" << std::endl;

        if ((numBytes - signed(this->ci.blockSize)) > 0) {
            readFromCache(response, tag, (index+1), (numBytes-ci.blockSize));
        }

        //this->programCache.at(index) = CacheEntry;

        return;
    }
}

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(index).size() == this->ci.associativity) {

    if ((this->ci.wp == WritePolicy::WriteBack) && (programCache.at(index).back().dirtyBit == true)) {
       if (this->level == this->ci.numCacheLevels) { 
        response->cycles += this->ci.memoryAccessCycles + numAddnAccess;
       }
        else {
            response->cycles += this->writeCycles() + numAddnAccess;
        }
    }
    if (ci.rp == ReplacementPolicy::LRU){
        programCache.at(index).pop_back();
    }

    else {
        int random = rand() % programCache.at(index).size();
        block = programCache.at(index).begin();
        advance(block, random);    
        programCache.at(index).erase(block);
    }


    response->evictions += 1;

    std::cout << "An eviction" << std::endl;
}

//Miss

BlockEntry requBlock {false,true,tag};

//Determine if the main memory will have to be accessed more than once 

//response->cycles += config.memoryAccessCycles + numAddnAccess;

programCache.at(index).push_front(requBlock);

//this->programCache.at(index) = CacheEntry;

response->misses += 1;

//cout << "Next cache is " << this->nextCache << endl;

if (this->level != this->ci.numCacheLevels) {
    this->nextCache->readFromCache(response, tag, index, numBytes); 
}
else {
    cout << "Accessing main memory" << endl;
    response->cycles += ci.memoryAccessCycles + numAddnAccess;
}

std::cout << "A miss" << std::endl;

cout << (numBytes - signed(this->ci.blockSize)) << endl;

if ((numBytes - signed(this->ci.blockSize)) > 0) {
    readFromCache(response, tag, (index+1), (numBytes-ci.blockSize));
}

return;

}

void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address, int numBytes) {

    // determine the index and tag
	AddressInfo ai = getAddressInfo(address);

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

    //Reset counters
    response->hits = 0;
    response->misses = 0;
    response->evictions = 0;

    response->cycles = 0;
    
    double requOps = ceil((double)numBytes / ci.blockSize);

    //for (int operations = 0; operations < int(requOps); operations++) {
    
    if (isWrite) {
        writeToCache(response, ai.tag, ai.setIndex, numBytes);
    }

    else {
        readFromCache(response, ai.tag, ai.setIndex, numBytes);
    }

    // "Response" data structure maintains counters of hits, misses, and evictions, so load those into global counters

    //}

    hits += response->hits;
    misses += response->misses;
    
    evictions += response->evictions; 

    cycles += response->cycles; 

	if (response->hits > 0)
		cout << "Operation at address " << std::hex << address << " caused " << response->hits << " hit(s)." << std::dec << endl;
	if (response->misses > 0)
		cout << "Operation at address " << std::hex << address << " caused " << response->misses << " miss(es)." << std::dec << endl;

	cout << "-----------------------------------------" << endl;

	return;
}

/*
	Starts reading the tracefile and processing memory operations.
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


	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		CacheResponse response;

		// ignore comments
		if (std::regex_match(line, commentPattern) || std::regex_match(line, instructionPattern)) {
			// skip over comments and CPU instructions
			continue;
		} 
        
        else if (std::regex_match(line, match, loadPattern)) {
			cout << "Found a load op!" << endl;
            globalRead++;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);
			
		} 
        
        else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
            globalWrite++;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, true, address, stoi(match.str(4)));
			logEntry(outfile, &response);
		} 
        
        else if (std::regex_match(line, match, modifyPattern)) {
			cout << "Found a modify op!" << endl;
			istringstream hexStream(match.str(2));
			// first process the read operation
            globalRead++;
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);
			outfile << endl;
			// now process the write operation
            globalWrite++;
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, true, address, stoi(match.str(4)));
			logEntry(outfile, &response);
		} 
        
        else {
			throw runtime_error("Encountered unknown line format in tracefile.");
		}
		outfile << endl;
	}


    for (unsigned int i = 0; i < ci.numCacheLevels; i++) { 

	    // add the final cache statistics
	    outfile << "L" << (i+1) << " Cache: Hits:" << hits << " Misses:" << misses << " Evictions:" << evictions << endl;

    }

	outfile << "Cycles:" << cycles << " Reads:" << globalRead << " Writes:" << globalWrite << endl;
	infile.close();
    
    cout << "Closing file" << endl;

    outfile.close();
}
	

/*
	Report the results of a memory access operation.
*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* response) {
	outfile << " " << response->cycles << " L1 ";	
	if (response->misses > 0)
		outfile << "miss ";
    if (response->hits > 0)
		outfile << "hit ";
	if (response->evictions > 0)
		outfile << "eviction";
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
	// this code should be changed to assign the proper index and tag

    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (this->ci.numByteOffsetBits + this->ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << this->ci.numByteOffsetBits << this->ci.numSetIndexBits)) >> this->ci.numByteOffsetBits; 

    //ai.setIndex = (address >> ci.numByteOffsetBits) && ci.numSetIndexBits; 

	return ai;
}


