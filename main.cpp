#include <thread>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <new>
#include <time.h>

// #define DEBUG2


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
	std::vector<double> percentUsersPerRegion;

	//user related
	User* regularUsers;
	int userCount;
	double reportChance;	
	int maxSingleUserBridgeAccessPerTimeInterval;
	int minSingleUserBridgeAccessPerTimeInterval;

	//bridge related
	BridgeAuthority* bridgeAuth;
	int minBridgeDBSize;
	int initBridgeCount;
	double geoIPErrorChance;
	double messageDropChance;

	//censor related
	double blockChance;

	//detector related
	Detector* blockDetector;
	int reportThreshold;
	int bridgeStatDiffThreshold;
	int numberOfDaysForAvgBridgeStats;

	volatile bool start;
	volatile bool done;
	

	uint64_t srandSeed; 
	//need array of rngs if we if we want concurrent sim
	Random64 rng;

	simGlobals () {
	};
};

std::vector<std::string> regionList;
std::vector<int> censorRegionIndexList;

std::vector<Censor*> censors;

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
		
		//parse censor region list
		getline(configFile,line);
		line = line.substr(line.find("=")+1, line.length());
		while (line.length() > 0) {		
			int delimPos = line.find(",");				
			int regionIndex;
			if (delimPos != std::string::npos) {
				regionIndex = find(regionList.begin(), regionList.end(), line.substr(0, delimPos)) - regionList.begin();
				censorRegionIndexList.push_back(regionIndex);
				line = line.substr(delimPos+1, line.length());
			}
			else {
				regionIndex = find(regionList.begin(), regionList.end(), line) - regionList.begin();
				censorRegionIndexList.push_back(regionIndex);
				break;
			}
		}

		getline(configFile,line);
		globals.reportThreshold = std::stoi(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.bridgeStatDiffThreshold = std::stoi(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.messageDropChance = std::stod(line.substr(line.find("=")+1, line.length()));		
		getline(configFile,line);
		globals.numberOfDaysForAvgBridgeStats = std::stoi(line.substr(line.find("=")+1, line.length()));		

		//parse percentage users per region list
		getline(configFile,line);		
		line = line.substr(line.find("=")+1, line.length());
		while (line.length() > 0) {		
			int delimPos = line.find(",");				
			if (delimPos != std::string::npos) {
				globals.percentUsersPerRegion.push_back(std::stod(line.substr(0, delimPos)));
				line = line.substr(delimPos+1, line.length());
			}
			else {
				globals.percentUsersPerRegion.push_back(std::stod(line));
				break;
			}
		}
		if (globals.percentUsersPerRegion.size() != 0 && globals.percentUsersPerRegion.size() != regionList.size()) {
			printf("ERROR: percentage of users per region list is non-empty and has size different from region list size\n");
			exit(-1);
		}

		getline(configFile,line);
		globals.minBridgeDBSize = std::stoi(line.substr(line.find("=")+1, line.length()));		

		configFile.close();
	}
}

void init() {		
	globals.regularUsers = (User*)malloc(sizeof(User) * globals.userCount);	
	globals.bridgeAuth = new BridgeAuthority(globals.initBridgeCount, globals.geoIPErrorChance, globals.messageDropChance, globals.minBridgeDBSize, &globals.rng);	

	for (int i = 0; i < censorRegionIndexList.size(); i++) {
		int censorRegionIndex = censorRegionIndexList[i];
		censors.push_back(new Censor(censorRegionIndex, globals.blockChance, &globals.rng));
	}
	
	globals.blockDetector = new Detector(globals.bridgeAuth, globals.reportThreshold, globals.bridgeStatDiffThreshold, globals.numberOfDaysForAvgBridgeStats, &globals.rng);

	// #pragma openmp parallel for
	for (int i = 0; i < globals.userCount; i++) {
		new (&globals.regularUsers[i]) User(globals.bridgeAuth, globals.blockDetector, globals.maxSingleUserBridgeAccessPerTimeInterval, globals.minSingleUserBridgeAccessPerTimeInterval, globals.reportChance, &globals.rng);
	}

	if (globals.percentUsersPerRegion.size() == 0) {
		//assign random regions to users
		for (int i = 0; i < globals.userCount; i++) {
			int randRegionIndex = globals.rng.next() % (regionList.size() - 1);
			globals.regularUsers[i].regionIndex = randRegionIndex;
		}
	}
	else {
		std::vector<int> usersPerRegion;
		int totalUsersSanityCheck = 0;
		for (int i = 0; i < globals.percentUsersPerRegion.size(); i++) {
			int usersInThisRegion = globals.userCount * (globals.percentUsersPerRegion[i] / 100.0);
			totalUsersSanityCheck += usersInThisRegion;
			usersPerRegion.push_back(usersInThisRegion);
		}

		if (totalUsersSanityCheck != globals.userCount) {
			printf("ERROR: summing users per region does not match total user count\n");
			printf("It is likely that the total user count does not divide evenly by the percentages of users per region\n");
			exit(-1);
		}

		int userIndex = 0;
		for (int i = 0; i < regionList.size(); i++) {
			for (int j = 0; j < usersPerRegion[i]; j++) {				
				globals.regularUsers[userIndex].regionIndex = i;
				userIndex++;
			}
		}
	}

	for (int regionIndex = 0; regionIndex < regionList.size(); regionIndex++) {
		globals.blockDetector->probesPerRegionIndex[regionIndex] = new Probe(regionIndex);
	}

	globals.start = false;
	globals.done = false;
}


void sequentialSim() {
	int updateCount = 0;	
	while(true) {
		//24H cycle for bridge stats
		if (updateCount > 0 && updateCount * globals.hoursPerUpdate % 24 == 0) {
			globals.bridgeAuth->publishDailyBridgeStats();
		}

		//currently bridge auth update is only used for debugging
		globals.bridgeAuth->update();

		//users connect to some number of bridges		
		for (int i = 0; i < globals.userCount; i++) {
			globals.regularUsers[i].update();			
		}

		//censor has a chance to block bridge
		for (int i = 0; i < censors.size(); i++) {
			censors[i]->update();
			printf("censored region accesses %d\n", censors[i]->numBridgeAccessesFromCensoredRegion);
		}

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

void printOutputs() {
	// printf("Reports per user:\n");
	// for (int i = 0; i < globals.userCount; i++) {
	// 	printf("%ld, ", globals.regularUsers[i].numReports);
	// }
	// printf("\n");
	// printf("Failed bridge accesses per user:\n");
	// for (int i = 0; i < globals.userCount; i++) {
	// 	printf("%ld, ", globals.regularUsers[i].numFailedBridgeAccesses);
	// }
	// printf("\n");

	printf("Total bridge added after initial bridge setup = %d\n", globals.bridgeAuth->numAddedBridges);

	uint64_t totalBridgeAccesses = 0;	
	for (int i = 0; i < globals.userCount; i++) {
		totalBridgeAccesses += globals.regularUsers[i].numBridgeAccesses;
	}
	printf("Total bridge accesses from all users/regions = %ld\n", totalBridgeAccesses);

	uint64_t totalBridgeAccessesFromCensoredRegions = 0;
	for (int i = 0; i < censors.size(); i++) {
		totalBridgeAccessesFromCensoredRegions += censors[i]->numBridgeAccessesFromCensoredRegion;
	}	
	printf("Total bridge accesses from censored regions = %ld\n", totalBridgeAccessesFromCensoredRegions);

	long totalCensorBlocks = 0;
	for (int i = 0; i < censors.size(); i++) {
		totalCensorBlocks += censors[i]->getBlockedBridgeCount();
	}
	printf("total_bridges_blocked_by_censor=%ld\n", totalCensorBlocks);
	printf("total_blocks_detected=%ld\n", globals.blockDetector->getDetectedBlockagesCount());

	printf("\n");
}

int main(int argc, char** argv) {
	std::string configFileRelativePath;
	bool useConfigFile = false;

    std::cout<<"binary="<<argv[0]<<std::endl;

	if (argc <= 1) {
		printf("Need to supply command line args.\n");
		printf("Example using config file:\n");
		printf("./exeName -f configFileRelativePath\n\n");

		printf("Example using command line args:\n");
		printf("./exeName -bCount 10 -blkChance 0.1 -repChance 0.3\n");

		exit(1);
	}

	//parse args
	//TODO add these to cmd line arg parse
	// 	iterationCount
	// hoursPerUpdate
	// totalUsers
	// maxSingleUserBridgeAccessPerTimeInterval
	// minSingleUserBridgeAccessPerTimeInterval
	// geoIPErrorChance
	// srandSee
	// regionList
	// censorRegion
	// reportThreshold
	// bridgeStatUsageDiffThreshold
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
	clock_t time = clock();	
	sequentialSim();
	time = clock() - time;
  	printf ("Approximate runtime of sim = %f seconds.\n", ((float)time)/CLOCKS_PER_SEC);
	printOutputs();
}