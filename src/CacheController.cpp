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

    //this->nextCache = new CacheController;// (struct CacheController*)malloc(sizeof(struct CacheController));
}



unsigned int CacheController::writeCycles(){

    int cycles = 0;

    CacheController *cache = this;

    while(cache->nextCache != NULL) {

        cycles += cache->ci.cacheAccessCycles;

        cache = cache->nextCache;
        
    }

    cout << "block" << cache->ci.blockSize << endl;

    //If main memory needs more than one read cycle

    return cycles;
}

void CacheController::writeToCache(CacheResponse* responses, unsigned long int address, int numBytes){

AddressInfo ai = getAddressInfo(address); 

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

cout << (this->ci.blockSize - 8) << endl;

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8));

//cout << "numaddnaccess is " << numAddnAccess << endl;

//if (numBytes < (signed)this->ci.blockSize) {

//    numBytes = (signed)this->ci.blockSize;

//}

//No matter what, we have to access the cache    
responses[(level-1)].cycles += ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = programCache.at(ai.setIndex).begin(); block != programCache.at(ai.setIndex).end(); block++) {

    AddressInfo blockAddr = getAddressInfo((*block).address); 

    if (blockAddr.tag == ai.tag) { // If the write attempt is a hit 
        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);
        responses[(level-1)].hits += 1;

        cout << "Back is " << programCache.at(ai.setIndex).back().address << endl;

        std::cout << "A Hit at index " << ai.setIndex << " tag " << ai.tag << std::endl;

        if (ci.wp == WritePolicy::WriteThrough) {

            //CacheController *cache = this->nextCache;

            cout << "stuff" << endl;

            if (this->level < this->ci.numCacheLevels) { 
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

        AddressInfo end = getAddressInfo(address + numBytes);
        
        if (end.setIndex != ai.setIndex) {

            cout << " Writing" << endl;

            int nextAddr = address + (1 << this->ci.numByteOffsetBits);
            writeToCache(responses, nextAddr, (numBytes-ci.blockSize));
        }
        
        return;
    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

cout << "Not a hit" << endl;

BlockEntry requBlock {false,true,address};

//if (ci.wp == WritePolicy::WriteThrough)
//    responses[(level-1)].cycles += writeCycles();


//cout << "Still nota hit" << endl;

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(ai.setIndex).size() == ci.associativity) {

    unsigned int evictAddr;

    if (ci.rp == ReplacementPolicy::LRU) {

        evictAddr = programCache.at(ai.setIndex).back().address;

        programCache.at(ai.setIndex).pop_back();
    }
    
    else {
        int random = rand() % programCache.at(ai.setIndex).size();
        block = programCache.at(ai.setIndex).begin();
        advance(block, random);   

        evictAddr = (*block).address;

        programCache.at(ai.setIndex).erase(block);

    }

    //cout << evictAddr << endl;

    AddressInfo evict = getAddressInfo(evictAddr);

    cout << "Evicting index" << evict.setIndex << " tag " << evict.tag << endl; 

    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (programCache.at(ai.setIndex).back().dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }
    else {
        //responses[(level-1)].cycles += this->nextCache->ci.cacheAccessCycles;
        this->nextCache->evictTo(evictAddr);
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

//If a lower-level cache, 

if (((this->level) != 1) && (this->ci.wp == WritePolicy::WriteThrough)) { 

    this->writeToCache(responses, address, numBytes);
}

if (((this->level) != this->ci.numCacheLevels) && (this->ci.wp == WritePolicy::WriteThrough)) {

    cout << "Write to next cache" << endl;

    responses[(level-1)].cycles += ci.cacheAccessCycles; 

    this->nextCache->writeToCache(responses, address, numBytes); 

}


else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess; // ci.cacheAccessCycles + numAddnAccess;
}

//Must update main memory after write is completed
//if ((config.wp == WritePolicy::WriteThrough) && (this->level  ) { 

AddressInfo end = getAddressInfo(address + numBytes);

if (end.setIndex != ai.setIndex) { 

    int nextAddr = address + (1 << this->ci.numByteOffsetBits);

    writeToCache(responses, nextAddr, (numBytes-ci.blockSize));
}


//responses[level].cycles += (ci.memoryAccessCycles + ci.cacheAccessCycles + numAddnAccess);


return;

}


void CacheController::readFromCache(CacheResponse* responses, unsigned long int address, int numBytes) {

cout << "Reading" << endl;

AddressInfo ai = getAddressInfo(address);

cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

std::list<BlockEntry> CacheEntry = programCache.at(ai.setIndex);

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
responses[(level-1)].cycles += this->ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

//cout << "size " << programCache.at(ai.setIndex).size() << endl;

//cout << *(programCache.at(index).begin()).ci.numByteOffsetBits << endl;

        //cout << "Back is index " << currBack.setIndex << " tag " << currBack.tag  << endl;
for (block = CacheEntry.begin(); block != CacheEntry.end(); block++) {

    //cout << "searching" << endl;

    AddressInfo blockAddr = getAddressInfo((*block).address); 
    
    if (blockAddr.tag == ai.tag) { // If the read attempt is a hit 
        
        cout << programCache.at(ai.setIndex).size() << endl;
        
        programCache.at(ai.setIndex).push_front(*block);
        programCache.at(ai.setIndex).erase(block);

        std::list<BlockEntry>::iterator b;

        for (b = CacheEntry.begin(); b != CacheEntry.end(); b++) { 

                AddressInfo a = getAddressInfo((*block).address);

                cout << "output index " << a.setIndex << " tag " << a.tag << endl;

        }

        responses[(level-1)].hits += 1;

        cout << programCache.at(ai.setIndex).size() << endl;
        
        std::cout << "A hit at index " << ai.setIndex << " tag " << ai.tag  << std::endl;

        //if ((numBytes - signed(this->ci.blockSize)) > 0) {
      
        AddressInfo end = getAddressInfo(address + numBytes);

        if (end.setIndex != ai.setIndex) { 

            cout << "Reading again" << endl;

            //cout << (numBytes - ci.blockSize) << endl;

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

    unsigned int evictAddr;

    if (ci.rp == ReplacementPolicy::LRU){

        evictAddr = programCache.at(ai.setIndex).back().address;

        cout << evictAddr << endl;

        programCache.at(ai.setIndex).pop_back();
    }

    else {
        int random = rand() % programCache.at(ai.setIndex).size();
        block = programCache.at(ai.setIndex).begin();
        advance(block, random);   

        evictAddr = (*block).address;
        //evictAddr = programCache.at(ai.setIndex).at

        programCache.at(ai.setIndex).erase(block);
    }

    if ((this->level) == this->ci.numCacheLevels) {
        if ((this->ci.wp == WritePolicy::WriteBack) && (programCache.at(ai.setIndex).back().dirtyBit == true)) {
            responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
        }
    }
    else {
        responses[(level-1)].cycles += this->nextCache->ci.cacheAccessCycles;
        this->nextCache->evictTo(evictAddr);
    }
    
    responses[(level-1)].evictions += 1;

    AddressInfo evict = getAddressInfo(evictAddr);

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

AddressInfo end = getAddressInfo(address + numBytes);

if (end.setIndex != ai.setIndex) { 
    cout << "Reading more blocks" << endl;   

    int nextAddr = address + (1 << this->ci.numByteOffsetBits);

    readFromCache(responses, nextAddr, (numBytes-ci.blockSize));
}

if ((this->level) < (this->ci.numCacheLevels)) {

    cout << "Reading from next cache" << endl;

    //int bytes = numBytes % (

    this->nextCache->readFromCache(responses, address, numBytes); 

}
else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;

    //int start = index - (index % 8);


    cout << "numbytes is" << numBytes << endl;

    //int num = int(ceil(numBytes/8));

}


return;

}


void CacheController::addTo(unsigned long int address) { 

    AddressInfo ai = getAddressInfo(address);

    BlockEntry block {false, true, address};
   
    cout << "Adding to " << this->level << " tag is " << ai.tag << "index is " << ai.setIndex << endl;

    cout << programCache.at(ai.setIndex).size() << endl;

    if (this->programCache.at(ai.setIndex).size() == ci.associativity) {

        cout << ci.numCacheLevels << endl;

        if ((level) < ci.numCacheLevels) {

            this->nextCache->evictTo(address);

        }
        
    }


    cout << "Pushing" << endl;

    this->programCache.at(ai.setIndex).push_front(block);

}

void CacheController::evictTo(unsigned long int address) {

    AddressInfo ai = getAddressInfo(address);

    cout << "Evicting" << endl;

    //cout << ci.associativity << endl;

    cout <<"Size is "<< this->programCache.at(ai.setIndex).size() << endl;

    if ((this->programCache.at(ai.setIndex).size() == ci.associativity)) {

        //If evicted from last cache, block simply disappears

        cout << ci.numCacheLevels << endl;

        if (level < ci.numCacheLevels) {    
            this->nextCache->evictTo(address);

        }
    }

    else {
       
        cout << "adding" << endl;

        this->addTo(address);

    }

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

    hits += responses[(level-1)].hits;

    misses += responses[(level-1)].misses;
    
    evictions += responses[(level-1)].evictions; 

    cycles += responses[(level-1)].cycles; 

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

	outfile << "Cycles:" << cycles << " Reads:" << globalRead << " Writes:" << globalWrite << endl;
	infile.close();
    
    cout << "Closing file" << endl;

    outfile.close();
}
	

/*
	Report the results of a memory access operation.
*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* responses) {

    int lineCycles = responses[0].cycles + responses[1].cycles + responses[2].cycles; 

    outfile << " " << (lineCycles); 
    for (unsigned int i = 0; i < ci.numCacheLevels; i++) {
        if ((responses[i].misses == 0) && (responses[i].hits == 0) && (responses[i].evictions == 0))
           break; 
	outfile << " L" << (i+1) << " ";	
	if (responses[i].misses > 0)
		outfile << "miss ";
    if (responses[i].hits > 0)
		outfile << "hit ";
	if (responses[i].evictions > 0)
		outfile << "eviction";
    }
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
	// this code should be changed to assign the proper index and tag


    //cout << "offset bits is " << this->ci.numByteOffsetBits << endl;

    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (this->ci.numByteOffsetBits + this->ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << this->ci.numByteOffsetBits << this->ci.numSetIndexBits)) >> this->ci.numByteOffsetBits; 

    //ai.setIndex = (address >> ci.numByteOffsetBits) && ci.numSetIndexBits; 

	return ai;
}


