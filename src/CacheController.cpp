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

void CacheController::writeToCache(CacheResponse* responses, unsigned long int tag, unsigned long int index, int numBytes){

cout << (this->ci.blockSize - 8) << endl;

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

if (numBytes < (signed)this->ci.blockSize) {

    numBytes = (signed)this->ci.blockSize;

}

//No matter what, we have to access the cache    
responses[(level-1)].cycles += ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

for (block = programCache.at(index).begin(); block != programCache.at(index).end(); block++) {

    if ((*block).tagBits == tag) { // If the write attempt is a hit 
        programCache.at(index).push_front(*block);
        programCache.at(index).erase(block);
        responses[(level-1)].hits += 1;

        std::cout << "A Hit" << std::endl;

        if (ci.wp == WritePolicy::WriteThrough) {

            //CacheController *cache = this->nextCache;

            cout << "stuff" << endl;

            if (this->level < this->ci.numCacheLevels) { 
                this->nextCache->writeToCache(responses, tag, index, numBytes);
            }

            else {
                cout << "adding mem" << endl;
                responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;
            }
            //while (cache->nextCache != NULL){

                cout << "Writing through" << endl;

                //cache->addTo(tag, index);
                //responses[cache->level].cycles += cache->ci.cacheAccessCycles;
            
                //cache = (cache->nextCache);

            //}
        }

        else {
            programCache.at(index).front().dirtyBit = true;
        }

        if ((numBytes - signed(this->ci.blockSize)) > 0) {

            cout << " Writing" << endl;

            writeToCache(responses, tag, (index+1), (numBytes-ci.blockSize));
        }
        
        return;
    }
}

//If it is not a hit, it is either a miss (and addition) or an eviction. 

cout << "Not a hit" << endl;

BlockEntry requBlock {false,true,tag};

if (ci.wp == WritePolicy::WriteThrough)
    responses[(level-1)].cycles += writeCycles();


cout << "Still nota hit" << endl;

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(index).size() == ci.associativity) {

    if ((ci.wp == WritePolicy::WriteBack) && (programCache.at(index).back().dirtyBit == true)) {
        std::cout << "Writing back" << std::endl; 
        responses[(level-1)].cycles += this->writeCycles();
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

    responses[(level-1)].evictions += 1;

    std::cout << "An eviction" << std::endl;

}

programCache.at(index).push_front(requBlock);

programCache.at(index).front().dirtyBit = true;

//this->programCache.at(index) = CacheEntry;

responses[(level-1)].misses += 1;

std::cout << "A Miss" << std::endl;

if (((this->level)) != this->ci.numCacheLevels) {

    cout << "Reading from next cache" << endl;

    responses[(level-1)].cycles += ci.cacheAccessCycles + numAddnAccess;

    this->nextCache->writeToCache(responses, tag, index, numBytes); 

}

else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += ci.memoryAccessCycles + ci.cacheAccessCycles + numAddnAccess;
}

//Must update main memory after write is completed
//if (config.wp == WritePolicy::WriteThrough) { 

if ((numBytes - signed(this->ci.blockSize)) > 0) {
    writeToCache(responses, tag, (index+1), (numBytes-ci.blockSize));
}


//responses[level].cycles += (ci.memoryAccessCycles + ci.cacheAccessCycles + numAddnAccess);


return;

}


void CacheController::readFromCache(CacheResponse* responses, unsigned long int tag, unsigned long int index, int numBytes) {

cout << "Reading" << endl;

//cout << this->programCache.size() << endl;
//cout << this->level << endl;

if (numBytes < (signed)this->ci.blockSize) {

    numBytes = (signed)this->ci.blockSize;

}

//cout << index << endl;

std::list<BlockEntry> CacheEntry = programCache.at(index);

int numAddnAccess = int((ceil(this->ci.blockSize - 8)/8)); 

//No matter what, we have to access the cache    
responses[(level-1)].cycles += this->ci.cacheAccessCycles;

std::list<BlockEntry>::iterator block;

cout << numBytes << endl;
//cout << ci.blockSize << endl;

//cout << CacheEntry.size() << endl;

cout << "size " << programCache.at(index).size() << endl;

//cout << *(programCache.at(index).begin()).ci.numByteOffsetBits << endl;

for (block = CacheEntry.begin(); block != CacheEntry.end(); block++) {

    //cout << "searching" << endl;

    if ((*block).tagBits == tag) { // If the read attempt is a hit 
        programCache.at(index).push_front(*block);
        programCache.at(index).erase(block);
        responses[(level-1)].hits += 1;

        std::cout << "A hit" << std::endl;

        if ((numBytes - signed(this->ci.blockSize)) > 0) {
            
            cout << "Reading again" << endl;

            cout << (numBytes - ci.blockSize) << endl;

            readFromCache(responses, tag, (index+1), (numBytes-ci.blockSize));
        }

        //this->programCache.at(index) = CacheEntry;

        return;
    }
}

//cout << "Not a hit" << endl;

//If this line of the cache is full, it must be an eviction
 
if (programCache.at(index).size() == this->ci.associativity) {

    if ((this->ci.wp == WritePolicy::WriteBack) && (programCache.at(index).back().dirtyBit == true)) {
       if ((this->level + 1) == this->ci.numCacheLevels) { 
        responses[(level-1)].cycles += this->ci.memoryAccessCycles + numAddnAccess;
       }
        else {
            responses[(level-1)].cycles += this->writeCycles();
            this->nextCache->evictTo(tag, index);
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

    responses[(level-1)].evictions += 1;

    std::cout << "An eviction" << std::endl;
}

//Miss

cout << tag << endl;

BlockEntry requBlock {false,true,tag};

//Determine if the main memory will have to be accessed more than once 

//responses[level].cycles += ci.memoryAccessCycles + numAddnAccess;

programCache.at(index).push_front(requBlock);

//this->programCache.at(index) = CacheEntry;

responses[(level-1)].misses += 1;

//cout << this->level << endl;


std::cout << "A miss" << std::endl;

cout << (numBytes - signed(this->ci.blockSize)) << endl;

if ((numBytes - signed(this->ci.blockSize)) > 0) {
    cout << "Reading more blocks" << endl;    
    readFromCache(responses, tag, (index+1), (numBytes-ci.blockSize));
}

if ((this->level) < (this->ci.numCacheLevels)) {

    cout << "Reading from next cache" << endl;

    //int bytes = numBytes % (

    this->nextCache->readFromCache(responses, tag, index, numBytes); 

}
else {
    cout << "Accessing main memory" << endl;
    responses[(level-1)].cycles += ci.memoryAccessCycles + numAddnAccess;

    //int start = index - (index % 8);


    cout << "numbytes is" << numBytes << endl;

    //int num = int(ceil(numBytes/8));

    for (int i = 0; i < (numAddnAccess+1); i++) {

        this->addTo(tag, i);    
    
    }
}


return;

}


void CacheController::addTo(unsigned long int tag, unsigned long int index) { 

    BlockEntry block {false, true, tag};
   
    cout << "Adding to " << this->level << " tag is " << tag << "index is " << index << endl;

    cout << programCache.at(index).size() << endl;

    if (this->programCache.at(index).size() == ci.associativity) {

        cout << ci.numCacheLevels << endl;

        if ((level) < ci.numCacheLevels) {

            this->nextCache->evictTo(tag, index);

        }
        
    }


    cout << "Pushing" << endl;

    this->programCache.at(index).push_front(block);

}

void CacheController::evictTo(unsigned long int tag, unsigned long int index) {

    cout << "Evicting" << endl;

    cout << ci.associativity << endl;
    cout << endl;

    cout << this->programCache.at(index).size() << endl;

    if ((this->programCache.at(index).size() == ci.associativity)) {

        //If evicted from last cache, block simply disappears

        cout << ci.numCacheLevels << endl;

        if (level < ci.numCacheLevels) {    
            this->nextCache->evictTo(tag, index);

        }
    }

    else {
       
        cout << "adding" << endl;

        this->addTo(tag, index);

    }

}

void CacheController::cacheAccess(CacheResponse *responses, bool isWrite, unsigned long int address, int numBytes) {

    // determine the index and tag
	AddressInfo ai = getAddressInfo(address);

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;


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
        writeToCache(responses, ai.tag, ai.setIndex, numBytes);
    }

    else {
        readFromCache(responses, ai.tag, ai.setIndex, numBytes);
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

    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (this->ci.numByteOffsetBits + this->ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << this->ci.numByteOffsetBits << this->ci.numSetIndexBits)) >> this->ci.numByteOffsetBits; 

    //ai.setIndex = (address >> ci.numByteOffsetBits) && ci.numSetIndexBits; 

	return ai;
}


