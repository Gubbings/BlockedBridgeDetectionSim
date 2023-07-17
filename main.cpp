#include <thread>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <new>
#include <time.h>

// #define DEBUG1
// #define DEBUG2
// #define DEBUG_SANITY


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
	double probeChancePercent;		
	double reportWeight;
	double bridgeStatsDiffWeight;
	double minConfidenceToProbe;
	int bridgeUsageThreshold;
	int numRetriesPerProbe;

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
		
		getline(configFile,line);
		globals.reportWeight = std::stod(line.substr(line.find("=")+1, line.length()));
		
		getline(configFile,line);
		globals.bridgeStatsDiffWeight = std::stod(line.substr(line.find("=")+1, line.length()));
		
		getline(configFile,line);
		globals.minConfidenceToProbe = std::stod(line.substr(line.find("=")+1, line.length()));
		
		getline(configFile,line);
		globals.bridgeUsageThreshold = std::stoi(line.substr(line.find("=")+1, line.length()));
				
		getline(configFile,line);
		globals.probeChancePercent = std::stod(line.substr(line.find("=")+1, line.length()));

		getline(configFile,line);
		globals.numRetriesPerProbe = std::stoi(line.substr(line.find("=")+1, line.length()));

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
	
	globals.blockDetector = new Detector(globals.bridgeAuth, globals.reportThreshold, globals.bridgeStatDiffThreshold, globals.numberOfDaysForAvgBridgeStats, globals.probeChancePercent, globals.reportWeight, globals.bridgeStatsDiffWeight, globals.minConfidenceToProbe, globals.bridgeUsageThreshold, globals.numRetriesPerProbe, &globals.rng);

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
#ifdef DEBUG1
			printf("censored region accesses %d\n", censors[i]->numBridgeAccessesFromCensoredRegion);
#endif
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
	uint64_t totalReports = 0;
	// printf("Reports per user:\n");
	for (int i = 0; i < globals.userCount; i++) {
		// printf("%ld, ", globals.regularUsers[i].numReports);
		totalReports += globals.regularUsers[i].numReports;
	}
	// printf("\n");
	printf("Total_user_reports=%ld\n", totalReports);

	// printf("Failed bridge accesses per user:\n");
	// for (int i = 0; i < globals.userCount; i++) {
	// 	printf("%ld, ", globals.regularUsers[i].numFailedBridgeAccesses);
	// }
	// printf("\n");

	printf("Total_bridge_added_after_initial_bridge_setup=%d\n", globals.bridgeAuth->numAddedBridges);

	uint64_t totalBridgeAccesses = 0;	
	for (int i = 0; i < globals.userCount; i++) {
		totalBridgeAccesses += globals.regularUsers[i].numBridgeAccesses;
	}
	printf("Total_bridge_accesses= %ld\n", totalBridgeAccesses);

	uint64_t totalBridgeAccessesFromCensoredRegions = 0;
	for (int i = 0; i < censors.size(); i++) {
		totalBridgeAccessesFromCensoredRegions += censors[i]->numBridgeAccessesFromCensoredRegion;
	}	
	printf("Total_bridge_accesses_from_censored_regions=%ld\n", totalBridgeAccessesFromCensoredRegions);

	long totalCensorBlocks = 0;
	for (int i = 0; i < censors.size(); i++) {
		totalCensorBlocks += censors[i]->getBlockedBridgeCount();
	}
	printf("total_bridges_blocked_by_censor=%ld\n", totalCensorBlocks);
	printf("total_blocks_detected=%ld\n", globals.blockDetector->getDetectedBlockagesCount());
	printf("total_suspicious_bridges=%ld\n", globals.blockDetector->getTotalSuspiciousBridgesCount());
	printf("total_probes_launched=%ld\n", globals.blockDetector->getNumLaunchedProbes());
	printf("total_times_bstats_daily_usage_dropped_below_threshold=%ld\n", globals.blockDetector->getBstatsDiffBelowThresholdCount());
	printf("total_times_bstats_diff_dropped_below_threshold=%ld\n", globals.blockDetector->getBstatsUsageBelowThresholdCount());


	printf("\n");
}

void printInputs() {
	printf("------------------------------------------------------------\n");

	printf("SIM RELATED INPUTS:\n");
	printf("####################################\n");
	printf("srandSeed=%ld\n", globals.srandSeed);
	printf("regionList=");
	for (int i = 0; i < regionList.size(); i++) {
		printf("%s,", regionList[i].c_str());
	}
	printf("\n");
	printf("censorRegionList=");
	for (int i = 0; i < censorRegionIndexList.size(); i++) {
		printf("%s,", regionList[censorRegionIndexList[i]].c_str());
	}
	printf("\n");
	printf("percentUsersPerRegion=");
	for (int i = 0; i < globals.percentUsersPerRegion.size(); i++) {
		printf("%f,", globals.percentUsersPerRegion[i]);
	}
	printf("\n");
	printf("geoIPErrorChance=%f\n", globals.geoIPErrorChance);
	printf("iterationCount=%ld\n", globals.iterationCount);
	printf("hoursPerUpdate=%d\n", globals.hoursPerUpdate);
	printf("messageDropChance=%f\n", globals.messageDropChance);
	printf("**********************************\n");

	printf("USERS RELATED INPUTS:\n");
	printf("####################################\n");
	printf("userCount=%d\n", globals.userCount);
	printf("reportChance=%f\n", globals.reportChance);
	printf("maxSingleUserBridgeAccessPerTimeInterval=%d\n", globals.maxSingleUserBridgeAccessPerTimeInterval);
	printf("minSingleUserBridgeAccessPerTimeInterval=%d\n", globals.minSingleUserBridgeAccessPerTimeInterval);
	printf("**********************************\n");

	printf("BRIDGE AUTHORITY RELATED INPUTS:\n");
	printf("####################################\n");
	printf("initBridgeCount=%d\n", globals.initBridgeCount);
	printf("minBridgeDBSize=%d\n", globals.minBridgeDBSize);
	printf("**********************************\n");

	printf("CENSOR RELATED INPUTS:\n");
	printf("####################################\n");
	printf("blockChance=%f\n", globals.blockChance);
	printf("**********************************\n");

	printf("DETECTOR RELATED INPUTS:\n");
	printf("####################################\n");
	printf("numRetriesPerProbe=%d\n", globals.numRetriesPerProbe);
	printf("numberOfDaysForAvgBridgeStats=%d\n", globals.numberOfDaysForAvgBridgeStats);
	printf("reportThreshold=%d\n", globals.reportThreshold);
	printf("bridgeStatDiffThreshold=%d\n", globals.bridgeStatDiffThreshold);		
	printf("bridgeUsageThreshold=%d\n", globals.bridgeUsageThreshold);
	printf("probeChancePercent=%f\n", globals.probeChancePercent);	
	printf("reportWeight=%f\n", globals.reportWeight);
	printf("bridgeStatsDiffWeight=%f\n", globals.bridgeStatsDiffWeight);
	printf("minConfidenceToProbe=%f\n", globals.minConfidenceToProbe);			
	printf("**********************************\n");
	
	printf("------------------------------------------------------------\n\n");
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
		printf("./exeName -i 1000 -hpu 12 -u 10000 -b 3500 -blkChance 0.05 -rChance 50 -maxBA 100 -minBA 20 -ge 10 -seed  -regions ca,ru,cn,us -censors cn -rt 5 -bst 50 -dropChance 10 -bsAvgDays 7 -upr 10,10,70,10 -minBDB 1000 -pChance 90 -rw 0.4 -bsdw 0.9 -mctp 0.9 -but 32 -rpp 10\n\n");

		exit(1);
	}

	//parse args
	globals.srandSeed = rand();
	for (int i=1;i<argc;++i) {
		if (strcmp(argv[i], "-f") == 0) {
            configFileRelativePath = argv[++i];
			useConfigFile = true;
			break;
        } 
		else if (strcmp(argv[i], "-i") == 0) {
			globals.iterationCount = std::stoull(argv[++i]);	
		}
		else if (strcmp(argv[i], "-hpu") == 0) {
			globals.hoursPerUpdate = std::stoull(argv[++i]);	
		}
		else if (strcmp(argv[i], "-u") == 0) {
			globals.userCount = std::stoi(argv[++i]);	
		}		
		else if (strcmp(argv[i], "-b") == 0) {
			globals.initBridgeCount = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-blkChance") == 0) {
			globals.blockChance = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-rChance") == 0) {
			globals.reportChance = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-maxBA") == 0) {
			globals.maxSingleUserBridgeAccessPerTimeInterval = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-minBA") == 0) {
			globals.minSingleUserBridgeAccessPerTimeInterval = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-ge") == 0) {
			globals.geoIPErrorChance = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-seed") == 0) {
			globals.srandSeed = std::stol(argv[++i]);
		}
		else if (strcmp(argv[i], "-regions") == 0) {			
			std::string line (argv[++i]);
			while (line.length() > 0) {		
				int delimPos = line.find(",");				
				if (delimPos != std::string::npos) {
					std::string r = line.substr(0, delimPos);
					regionList.push_back(r);
					line = line.substr(delimPos+1, line.length());
				}
				else {
					regionList.push_back(line);
					break;
				}
			}
		}
		else if (strcmp(argv[i], "-censors") == 0) {			
			std::string line (argv[++i]);
			while (line.length() > 0) {		
				int delimPos = line.find(",");				
				int regionIndex;
				if (delimPos != std::string::npos) {
					std::string r = line.substr(0, delimPos);
					regionIndex = find(regionList.begin(), regionList.end(), r) - regionList.begin();
					censorRegionIndexList.push_back(regionIndex);
					line = line.substr(delimPos+1, line.length());
				}
				else {
					regionIndex = find(regionList.begin(), regionList.end(), line) - regionList.begin();
					censorRegionIndexList.push_back(regionIndex);
					break;
				}
			}
		}
		else if (strcmp(argv[i], "-rt") == 0) {
			globals.reportThreshold = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-bst") == 0) {
			globals.bridgeStatDiffThreshold = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-dropChance") == 0) {
			globals.messageDropChance = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-bsAvgDays") == 0) {
			globals.numberOfDaysForAvgBridgeStats = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-upr") == 0) {			
			std::string line (argv[++i]);
			while (line.length() > 0) {		
				int delimPos = line.find(",");				
				if (delimPos != std::string::npos) {
					double d = std::stod(line.substr(0, delimPos));
					globals.percentUsersPerRegion.push_back(d);
					line = line.substr(delimPos+1, line.length());
					printf("%f,", d);
				}
				else {
					globals.percentUsersPerRegion.push_back(std::stod(line));
					printf("%f,", std::stod(line));
					break;
				}
			}
			printf("\n");
		}
		else if (strcmp(argv[i], "-minBDB") == 0) {
			globals.minBridgeDBSize = std::stoi(argv[++i]);
		}

		else if (strcmp(argv[i], "-pChance") == 0) {
			globals.probeChancePercent = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-rw") == 0) {
			globals.reportWeight = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-bsdw") == 0) {
			globals.bridgeStatsDiffWeight = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-mctp") == 0) {
			globals.minConfidenceToProbe = std::stod(argv[++i]);
		}
		else if (strcmp(argv[i], "-but") == 0) {
			globals.bridgeUsageThreshold = std::stoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-rpp") == 0) {
			globals.numRetriesPerProbe = std::stoi(argv[++i]);
		}
		else {
            std::cout<<"ERROR: bad argument "<<argv[i]<<std::endl;
            exit(1);
        }
	}	

	if (useConfigFile) {
		parseConfigFile(configFileRelativePath);
	}
	else {
		globals.rng.setSeed(globals.srandSeed);
	}

	init();
	printInputs();
	clock_t time = clock();	
	sequentialSim();
	time = clock() - time;
  	printf ("Approximate runtime of sim = %f seconds.\n", ((float)time)/CLOCKS_PER_SEC);
	printOutputs();
}