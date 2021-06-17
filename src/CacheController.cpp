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
   
    cout << ci.numberSets << endl;  

    for (unsigned int i = 0; i < ci.numberSets; i++) {

        std::list<BlockEntry> CacheEntry;

        //cout << "Size is " << CacheEntry.size() << endl;

        programCache.push_back(CacheEntry); 

    }

    this->programCache = programCache;
}

void CacheController::writeToCache(CacheResponse* responses, unsigned long int address, int numBytes){

AddressInfo ai = getAddressInfo(address); 

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8));

//No matter what, we have to access the cache  

responses[(level-1)].cycles += ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

//cout << "Size is " << programCache.at(ai.setIndex).size() << endl;

cout << "blocksize is " << this->ci.blockSize << endl;

cout << "numbytes is " << numBytes << endl;

for (block = programCache.at(ai.setIndex).begin(); block != programCache.at(ai.setIndex).end(); block++) {

    AddressInfo blockAddr = getAddressInfo((*block).address); 

    if (blockAddr.tag == ai.tag) { // If the write attempt is a hit 
        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);
        responses[(level-1)].hits += 1;


        std::cout << "A Hit at index " << ai.setIndex << " tag " << ai.tag << std::endl;

        if (ci.wp == WritePolicy::WriteThrough) {

            //CacheController *cache = this->nextCache;

            cout << "writing through" << endl;

            if (this->level < this->ci.numCacheLevels) {
                //responses[(level)].cycles += this->nextCache->ci.cacheAccessCycles; 
                this->nextCache->writeToCache(responses, address, numBytes);
            }

            else {
                cout << "adding mem" << endl;
                responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;
            }
        }

        else {
            programCache.at(ai.setIndex).front().dirtyBit = true;
        }
        
        int overflow = numBytes - ci.blockSize;

        cout << "overflow is " << overflow << endl;

        if (overflow > 0) {

            cout << " Writing to overflow" << endl;

            int nextAddr = address + (1 << this->ci.numByteOffsetBits);
            writeToCache(responses, nextAddr, overflow);
        }


    return;

    }


}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

cout << "Not a hit" << endl;

BlockEntry requBlock {false,true,address};

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(ai.setIndex).size() == ci.associativity) {

    BlockEntry evicted;

    if (ci.rp == ReplacementPolicy::LRU) {

        evicted = programCache.at(ai.setIndex).back();

        programCache.at(ai.setIndex).pop_back();
    }
    
    else {
        int random = rand() % programCache.at(ai.setIndex).size();
        block = programCache.at(ai.setIndex).begin();
        advance(block, random);   

        evicted = (*block);

        programCache.at(ai.setIndex).erase(block);

    }

    AddressInfo evict = getAddressInfo(evicted.address);

    cout << "Evicting index" << evict.setIndex << " tag " << evict.tag << endl; 

    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (evicted.dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }
    else {
        //responses[(level-1)].cycles += this->nextCache->ci.cacheAccessCycles;
        this->nextCache->evictTo(evicted);
    }


    responses[(level-1)].evictions += 1;

    std::cout << "An eviction" << std::endl;

}

//Add new block to cache at index

programCache.at(ai.setIndex).push_front(requBlock);

programCache.at(ai.setIndex).front().dirtyBit = true;

responses[(level-1)].misses += 1;

std::cout << "A Miss" << std::endl;


//Write-through

if (((this->level) != 1)) {
    
    if (this->ci.wp == WritePolicy::WriteThrough) { 

    //Always a hit if there was a previous miss on the same operation
    responses[(level-1)].hits += 1;

    cout << "Already been here" << endl;
    }
    //responses[(level-1)].cycles += ci.cacheAccessCycles;
}


else { 

    //If cache is L1, ensure cycles for probing and writing are counted

    responses[(level-1)].cycles += this->ci.cacheAccessCycles;
}

cout << this->level << endl;

//responses[(level-1)].cycles += ci.cacheAccessCycles;

if ((this->level) < this->ci.numCacheLevels) {

    cout << "Write to next cache" << endl;

    responses[(level)].cycles += this->nextCache->ci.cacheAccessCycles; 

    this->nextCache->writeToCache(responses, address, numBytes); 

}


else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += (ci.memoryAccessCycles + numAddnAccess);

    //responses[(level-1)].cycles += ci.cacheAccessCycles;

    if (this->ci.wp == WritePolicy::WriteThrough) {

        responses[(level-1)].cycles += (ci.memoryAccessCycles + numAddnAccess);

    }
}

//else {

//    responses[(level-1)].cycles += (ci.cacheAccessCycles + ci.memoryAccessCycles + numAddnAccess);

//}

int overflow = numBytes - ci.blockSize;

if (overflow > 0) { 

    int nextAddr = address + (1 << this->ci.numByteOffsetBits);

    writeToCache(responses, nextAddr, overflow);
}

return;

}


void CacheController::readFromCache(CacheResponse* responses, unsigned long int address, int numBytes) {

cout << "Reading" << endl;

AddressInfo ai = getAddressInfo(address);

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
responses[(level-1)].cycles += this->ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

cout << "size " << programCache.at(ai.setIndex).size() << endl;

for (block = programCache.at(ai.setIndex).begin(); block != programCache.at(ai.setIndex).end(); block++) {

    //cout << "searching" << endl;

    AddressInfo blockAddr = getAddressInfo((*block).address);

    cout << "output index " << blockAddr.setIndex << " tag " << blockAddr.tag << endl;

    
    if (blockAddr.tag == ai.tag) { // If the read attempt is a hit 
        
        cout << programCache.at(ai.setIndex).size() << endl;
        
        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);
        
        
        //cout << programCache.at(ai.setIndex).size() << endl;

        responses[(level-1)].hits += 1;

        //cout << programCache.at(ai.setIndex).size() << endl;
        
        std::cout << "A hit at index " << ai.setIndex << " tag " << ai.tag  << std::endl;

        cout << "numbytes is " << numBytes << endl;

        AddressInfo end = getAddressInfo(address + numBytes);

        cout << "End addr is index" << end.setIndex << " tag " << end.tag << endl;

        int overflow = numBytes - ci.blockSize;

        if (overflow > 0) { 

            cout << "Reading again" << endl;

            int nextAddr = address + (1 << this->ci.numByteOffsetBits);

            readFromCache(responses, nextAddr, (numBytes-ci.blockSize));
        }

        return;
    }
}

cout << "Not a hit" << endl;

//If this line of the cache is full, it must be an eviction

cout << "size at index is " << programCache.at(ai.setIndex).size() << endl;
 
if (programCache.at(ai.setIndex).size() == this->ci.associativity) {

    BlockEntry evicted;

    if (ci.rp == ReplacementPolicy::LRU){

        evicted = programCache.at(ai.setIndex).back();

        programCache.at(ai.setIndex).pop_back();
    }

    else {
        int random = rand() % programCache.at(ai.setIndex).size();
        block = programCache.at(ai.setIndex).begin();
        advance(block, random);   

        evicted = (*block);
        //evictAddr = programCache.at(ai.setIndex).at

        programCache.at(ai.setIndex).erase(block);
    }

    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (evicted.dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }
    else {
        //responses[(level-1)].cycles += this->nextCache->ci.cacheAccessCycles;
        this->nextCache->evictTo(evicted);
    }
    
    responses[(level-1)].evictions += 1;

    AddressInfo evict = getAddressInfo(evicted.address);

    cout << "Evicting index" << evict.setIndex << " tag " << evict.tag << endl; 
}

//Miss

//cout << ai.tag << endl;

BlockEntry requBlock {false,true,address};


//Determine if the main memory will have to be accessed more than once 

//responses[level].cycles += ci.memoryAccessCycles + numAddnAccess;

programCache.at(ai.setIndex).push_front(requBlock);

responses[(level-1)].misses += 1;

std::cout << "A miss" << std::endl;

//cout << (numBytes - signed(this->ci.blockSize)) << endl;

//AddressInfo end = getAddressInfo(address + numBytes);

int overflow = numBytes - ci.blockSize;

cout << "overflow is " << overflow << endl;

if (overflow > 0) { 
    cout << "Reading more blocks" << endl;   

    int nextAddr = address + (1 << this->ci.numByteOffsetBits);

    readFromCache(responses, nextAddr, overflow);
}

if ((this->level) < (this->ci.numCacheLevels)) {

    cout << "Reading from next cache" << endl;

    this->nextCache->readFromCache(responses, address, numBytes); 

}
else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;
}


return;

}


void CacheController::addTo(BlockEntry evicted) { 

    AddressInfo ai = getAddressInfo(evicted.address);

    BlockEntry block = evicted;// {false, true, address};
  
    block.dirtyBit = false;

    cout << "Adding to " << this->level << " tag is " << ai.tag << " index is " << ai.setIndex << endl;

    cout << programCache.at(ai.setIndex).size() << endl;

    if (this->programCache.at(ai.setIndex).size() == ci.associativity) {

        cout << ci.numCacheLevels << endl;

        if ((level) < ci.numCacheLevels) {

            this->nextCache->evictTo(block);

        }
        
    }


    cout << "Pushing" << endl;

    this->programCache.at(ai.setIndex).push_front(block);

    return;

}

void CacheController::evictTo(BlockEntry evicted) {

    AddressInfo ai = getAddressInfo(evicted.address);

    cout << "Evicting" << endl;

    //cout << ci.associativity << endl;

    cout <<"Size is "<< this->programCache.at(ai.setIndex).size() << endl;

    std::list<BlockEntry>::iterator block;

    for (block = this->programCache.at(ai.setIndex).begin(); block != this->programCache.at(ai.setIndex).end(); block++) {

        if ((*block).address == evicted.address) {

            return;

        }

    }

    if ((this->programCache.at(ai.setIndex).size() == ci.associativity)) {

        //If evicted from last cache, block simply disappears

        cout << ci.numCacheLevels << endl;

        if (level < ci.numCacheLevels) {    
            this->nextCache->evictTo(evicted);

        }
    }

    else {
       
        cout << "adding" << endl;

        this->addTo(evicted);

    }

    return;
}

void CacheController::cacheAccess(CacheResponse *responses, bool isWrite, unsigned long int address, int numBytes) {

    // determine the index and tag
	//AddressInfo ai = getAddressInfo(address);



    for (unsigned int i = 0; i < 3; i++) {

        //Reset counters
        responses[i].hits = 0;
        responses[i].misses = 0;
        responses[i].evictions = 0;

        responses[i].cycles = 0;
    
    }

    //double requOps = ceil((double)numBytes / ci.blockSize);

    //for (int operations = 0; operations < int(requOps); operations++) {
    
    if (isWrite) {
        writeToCache(responses, address, numBytes);
    }

    else {
        readFromCache(responses, address, numBytes);
    }

    // "Response" data structure maintains counters of hits, misses, and evictions, so load those into global counters

    //}


	if (responses[(level-1)].hits > 0)
		cout << "Operation at address " << std::hex << address << " caused " << responses[(level-1)].hits << " hit(s)." << std::dec << endl;
	if (responses[(level-1)].misses > 0)
		cout << "Operation at address " << std::hex << address << " caused " << responses[(level-1)].misses << " miss(es)." << std::dec << endl;

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

    CacheResponse responses[3];

    //int globalRead = 0;
    //int globalWrite = 0;
    //int cycles = 0;

	// parse each line of the file and look for commands
	while (getline(infile, line)) {
		// these strings will be used in the file output
		string opString, activityString;
		smatch match; // will eventually hold the hexadecimal address string
		unsigned long int address;
		// create a struct to track cache responses
		//CacheResponse response;

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


    CacheController *cont = this;

    for (unsigned int i = 0; i < ci.numCacheLevels; i++) { 

	    // add the final cache statistics
	    outfile << "L" << (i+1) << " Cache: Hits:" << cont->hits << " Misses:" << cont->misses << " Evictions:" << cont->evictions << endl;

        cont = cont->nextCache;

    }

	outfile << "Cycles:" << this->cycles << " Reads:" << globalRead << " Writes:" << globalWrite << endl;
	infile.close();
    
    cout << "Closing file" << endl;

    outfile.close();
}
	

/*
	Report the results of a memory access operation.
*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* responses) {

    int lineCycles = responses[0].cycles + responses[1].cycles + responses[2].cycles; 

    this->cycles += lineCycles;

    CacheController *cache = this;

    outfile << " " << (lineCycles) << " "; 
    for (unsigned int i = 0; i < ci.numCacheLevels; i++) {

    if ((responses[i].misses == 0) && (responses[i].hits == 0) && (responses[i].evictions == 0))
           break; 
	
    outfile << "L" << (i+1) << " ";	
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
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
    //cout << "offset bits is " << this->ci.numByteOffsetBits << endl;

    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (this->ci.numByteOffsetBits + this->ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << this->ci.numByteOffsetBits << this->ci.numSetIndexBits)) >> this->ci.numByteOffsetBits; 

	return ai;
}


