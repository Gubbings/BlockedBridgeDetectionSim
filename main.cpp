#include <thread>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <new>
#include "bridge.h"
#include "bridgeAuthority.h"
#include "detector.h"
#include "user.h"
#include "censor.h"
#include "xoshiro_srand.h"

__thread int tid;

//TODO needs padding
struct simGlobals {
	//sim stuff
	uint64_t iterationCount;
	int hoursPerUpdate;

	//user related
	int userCount;
	double reportChance;	
	int maxSingleUserBridgeAccessPerTimeInterval;
	int minSingleUserBridgeAccessPerTimeInterval;

	//bridge related
	int initBridgeCount;
	double geoIPErrorChance;
	
	//censor related
	double blockChance;
	std::string censorRegion;

	//detector related
	int reportThreshold;
	int bridgeStatDiffThreshold;
	

	volatile bool start;
	volatile bool done;

	User* regularUsers;
	BridgeAuthority* bridgeAuth;
	Detector* blockDetector;
	Probe* probes;
	Censor* censor;

	uint64_t srandSeed; 
	//need array for multithread
	Random64 rng;

	simGlobals () {
	};
};

std::vector<std::string> regionList;


simGlobals globals;
void parseConfigFile(std::string &configFileRelativePath) {
	std::string line;
	std::ifstream configFile (configFileRelativePath);
	if (configFile.is_open()) {
		//parse numeric config inputs
		getline(configFile,line);		
		globals.iterationCount = std::stoull(line.substr(line.find("=")+1, line.length()));
		getline(configFile,line);		
		globals.hoursPerUpdate = std::stoull(line.substr(line.find("=")+1, line.length()));
		getline(configFile,line);		
		globals.userCount = std::stoi(line.substr(line.find("=")+1, line.length()));
		getline(configFile,line);		
		globals.initBridgeCount = std::stoi(line.substr(line.find("=")+1, line.length()));
		getline(configFile,line);
		globals.blockChance = std::stod(line.substr(line.find("=")+1, line.length()));
		getline(configFile,line);
		globals.reportChance = std::stod(line.substr(line.find("=")+1, line.length()));		
		
		getline(configFile,line);
		globals.maxSingleUserBridgeAccessPerTimeInterval = std::stoi(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.minSingleUserBridgeAccessPerTimeInterval = std::stoi(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.geoIPErrorChance = std::stod(line.substr(line.find("=")+1, line.length()));		

		
		getline(configFile,line);
		line = line.substr(line.find("=")+1, line.length());
		if (line.length() > 0) {
			globals.srandSeed = std::stol(line);		
		}
		else {
			globals.srandSeed = rand();					
		}
		globals.rng.setSeed(globals.srandSeed);
		printf("srandSeed=%ld\n", globals.srandSeed);


		//parse region list
		getline(configFile,line);		
		line = line.substr(line.find("=")+1, line.length());
		while (line.length() > 0) {		
			int delimPos = line.find(",");				
			if (delimPos != std::string::npos) {
				regionList.push_back(line.substr(0, delimPos));
				line = line.substr(delimPos+1, line.length());
			}
			else {
				regionList.push_back(line);
				break;
			}
		}
		
		getline(configFile,line);
		line = line.substr(line.find("=")+1, line.length());
		globals.censorRegion = line;
		getline(configFile,line);
		globals.reportThreshold = std::stoi(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.bridgeStatDiffThreshold = std::stoi(line.substr(line.find("=")+1, line.length()));		

		configFile.close();
	}
}

void init() {		
	globals.regularUsers = (User*)malloc(sizeof(User) * globals.userCount);	
	globals.bridgeAuth = new BridgeAuthority(globals.initBridgeCount, globals.geoIPErrorChance, &globals.rng);	

	int censorRegionIndex = find(regionList.begin(), regionList.end(), globals.censorRegion) - regionList.begin();
	globals.censor = new Censor(censorRegionIndex, globals.blockChance, &globals.rng);
	globals.blockDetector = new Detector(globals.bridgeAuth, globals.censor, globals.reportThreshold, globals.bridgeStatDiffThreshold);

	for (int i = 0; i < globals.userCount; i++) {
		new (&globals.regularUsers[i]) User(globals.bridgeAuth, globals.blockDetector, globals.censor, globals.maxSingleUserBridgeAccessPerTimeInterval, globals.minSingleUserBridgeAccessPerTimeInterval, globals.reportChance, &globals.rng);
	}

	//assign random regions to users
	for (int i = 0; i < globals.userCount; i++) {
		int randRegionIndex = globals.rng.next() % (regionList.size() - 1);
		globals.regularUsers[i].regionIndex = randRegionIndex;
	}

	for (int regionIndex = 0; regionIndex < regionList.size(); regionIndex++) {
		globals.blockDetector->probesPerRegionIndex[regionIndex] = new Probe(regionIndex, globals.censor);
	}

	globals.start = false;
	globals.done = false;
}


// void regularUserThread() {
// }

// void censorThread() {
// }

// void detectorThread() {
// 	while(!globals.done) {
// 	}
// }


// void concurrentSim() {
// 	std::thread* userThreads;
// 	std::thread censorThread;
// 	std::thread detectorThread;
// }

void sequentialSim() {
	int updateCount = 0;	
	while(true) {
		//24H cycle for bridge stats
		if (updateCount > 0 && updateCount * globals.hoursPerUpdate % 24 == 0) {
			globals.bridgeAuth->publishDailyBridgeStats();
		}

		//users connect to some number of bridges
		for (int i = 0; i < globals.userCount; i++) {
			globals.regularUsers[i].update();
		}

		//censor has a chance to block bridge
		globals.censor->update();

		//detector does any independant work with bridge stats
		globals.blockDetector->update();
		
		updateCount++;
		if (updateCount % 1000 == 0) {
			if (updateCount >= globals.iterationCount){
				break;
			}
		}
	}
}


int main(int argc, char** argv) {
	std::string configFileRelativePath;
	bool useConfigFile = false;

    std::cout<<"binary="<<argv[0]<<std::endl;

	//parse args
	for (int i=1;i<argc;++i) {
		if (strcmp(argv[i], "-f") == 0) {
            configFileRelativePath = argv[++i];
			useConfigFile = true;
			break;
        } 
		else if (strcmp(argv[i], "-bCount") == 0) {
			globals.initBridgeCount = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-blkChance") == 0) {
			globals.blockChance = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-repChance") == 0) {
			globals.reportChance = std::stoi(argv[++i]);
		}
		else {
            std::cout<<"bad argument "<<argv[i]<<std::endl;
            exit(1);
        }
	}

	if (useConfigFile) {
		parseConfigFile(configFileRelativePath);
	}

#ifndef NO_DEBUG
	printf("Bridge Count = %d\n", globals.initBridgeCount);
	printf("Block Chance = %f\n", globals.blockChance);
	printf("Report Chance = %f\n", globals.reportChance);
#endif

	init();
	sequentialSim();
}