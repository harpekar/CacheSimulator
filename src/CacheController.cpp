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

#include "CacheController.h"
#include "BlockEntry.h"
#include "CacheMod.h"


using namespace std;

CacheController::CacheController(CacheInfo ci, string tracefile) {
	// store the configuration info
	this->ci = ci;
	this->inputFile = tracefile;
	this->outputFile = this->inputFile + ".out";
	// compute the other cache parameters


	this->ci.numByteOffsetBits = log2(ci.blockSize);
	this->ci.numSetIndexBits = log2(ci.numberSets);
	
    // initialize the counters
    this->globalCycles = 0;
	this->globalHits = 0;
	this->globalMisses = 0;
	this->globalEvictions = 0;

    //this->writeNum = 0;
    //this->readNum = 0;


    //Initialize Cache
    std::vector<std::list<BlockEntry>> programCache;

    //Initialize each cache block at each address with the given associativity level
    
    for (size_t i = 0; i < ci.numberSets; i++) {

        std::list <BlockEntry> CacheEntry; 
        programCache.push_back(CacheEntry); 

    }

    this->programCache = programCache;

}

void CacheController::cacheAccess(CacheResponse* response, bool isWrite, unsigned long int address, int numBytes) {
 
    //Reset counters
    response->hits = 0;
    response->misses = 0;
    response->evictions = 0;

    response->cycles = 0;

    // determine the index and tag
	AddressInfo ai = getAddressInfo(address);

    unsigned int cycleNumber = 0;

	cout << "\tSet index: " << ai.setIndex << ", tag: " << ai.tag << endl;

    if (isWrite) {
        writeToCache(programCache.at(ai.setIndex), ai.tag, ci, response);
    }

    else {
        readFromCache(programCache.at(ai.setIndex), ai.tag, ci, response);
    }

    // "Response" data structure maintains counters of hits, misses, and evictions, so load those into global counters

    globalHits += int(response->hits);
    globalMisses += int(!response->hits);
    
    globalEvictions += int(response->evictions); 

    globalCycles += response->cycles; 


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
	cout << "Input tracefile: " << inputFile << endl;
	cout << "Output file name: " << outputFile << endl;
	
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
            readNum++;
			istringstream hexStream(match.str(2));
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);
			
		} 
        
        else if (std::regex_match(line, match, storePattern)) {
			cout << "Found a store op!" << endl;
            writeNum++;
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
            //readNum++;
			hexStream >> std::hex >> address;
			outfile << match.str(1) << match.str(2) << match.str(3) << match.str(4);
			cacheAccess(&response, false, address, stoi(match.str(4)));
			logEntry(outfile, &response);
			outfile << endl;
			// now process the write operation
            //writeNum++;
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


	// add the final cache statistics
	outfile << "L1 Cache: Hits:" << globalHits << " Misses:" << globalMisses << " Evictions:" << globalEvictions << endl;
	outfile << "Cycles:" << globalCycles << " Reads:" << readNum << " Writes:" << writeNum << endl;

	infile.close();
	outfile.close();
}

/*
	Report the results of a memory access operation.
*/
void CacheController::logEntry(ofstream& outfile, CacheResponse* response) {
	outfile << " " << response->cycles;
	if (response->hits > 0)
		outfile << " hit";
	if (response->misses > 0)
		outfile << " miss";
	if (response->evictions > 0)
		outfile << " eviction";
}

/*
	Calculate the block index and tag for a specified address.
*/
CacheController::AddressInfo CacheController::getAddressInfo(unsigned long int address) {
	AddressInfo ai;
	// this code should be changed to assign the proper index and tag

    //Isolate Tag bits (end of address) by shifting   
    ai.tag = address >> (ci.numByteOffsetBits + ci.numSetIndexBits);

    //Isolate Index bits by subtracting off Tag and then shifting
    ai.setIndex = (address - (ai.tag << (ci.numByteOffsetBits + ci.numSetIndexBits))) >> ci.numByteOffsetBits; 

	return ai;
}


