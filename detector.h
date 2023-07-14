#pragma once

#include "probe.h"
#include "censor.h"
#include "bridge.h"
#include "bridgeAuthority.fwd.h"
#include "xoshiro_srand.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <math.h>       


#define MAX_NUM_REGIONS 8
extern std::vector<std::string> regionList;
extern std::vector<int> censorRegionIndexList;

struct ReportPair {
	Bridge* bridge;
	int regionIndex;

	bool operator=(const ReportPair &other) const {
        return bridge == other.bridge && regionIndex == other.regionIndex;
    }

    bool operator<(const ReportPair &other) const {
        return regionIndex < other.regionIndex;
    }
	
	ReportPair(Bridge* _b, int _ri) {
		bridge = _b;
		regionIndex = _ri;
	}

	ReportPair(ReportPair const &other) {
		bridge = other.bridge;
		regionIndex = other.regionIndex;
	}

	~ReportPair(){
	}
};

struct ReportPairStackNode {
	ReportPairStackNode* next;
	ReportPair* rp;

	ReportPairStackNode(ReportPairStackNode* _next, ReportPair* _rp) {
		next = _next;
		rp = _rp;
	}
};

class Detector {
private:
	BridgeAuthority* bridgeAuth;
	Random64* rng;
	std::map<Bridge*, int> numReportsPerBridgeRegionIndex[MAX_NUM_REGIONS];

	int launchedProbes = 0;
	long blockedBridgeDetectionCount = 0;

	int reportThreshold;
	int bridgeStatDiffThreshold;	
	int numDaysHistoricalBridgeStatsAvg;

	long bridgeStatsDailyDiff(Bridge* b, int regionIndex) {
		int currUsageMultipleOf8 = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		int prevUsageMultipleOf8 = b->getHistoricalDailyUsageFromRegionIndex(1, regionIndex);				
		return prevUsageMultipleOf8 - currUsageMultipleOf8;
	}

	long bridgeStatsHistoricalAvgDiff(Bridge* b, int regionIndex) {
		int totalHistoricalUsage = 0;
		for (int i = 1; i <= numDaysHistoricalBridgeStatsAvg; i++) {
			totalHistoricalUsage += b->getHistoricalDailyUsageFromRegionIndex(i, regionIndex);
		}
		int historicalAverage = ceil(totalHistoricalUsage / (numDaysHistoricalBridgeStatsAvg * 1.0));
		int prevUsageMultipleOf8 = (historicalAverage + 7) & (-8);

		int currUsageMultipleOf8 = b->getCurrentDailyUsageFromRegionIndex(regionIndex);

		return prevUsageMultipleOf8 - currUsageMultipleOf8;
	}

public:
	std::map<int, Probe*> probesPerRegionIndex;
	// std::vector<ReportPair*> unresolvedReportPairs;
	// ReportPairStackNode* unresolvedReportPairs;
	// std::map<ReportPair*, int> numReportsPerBridgeRegion;	
	std::vector<Bridge*> blockedBridges;

	//TODO fix erasing from these vectors
	//These 2 vectors are always updated togther, which might suggest that we should have
	// a list of structs/pairs instead however this complicates getting the list of blocked bridges 
	// without iterating the regions so instead we will always update these 2 vectors together
	// this is a dumb way to do this but it suffices for now
	// std::vector<Bridge*> suspectedBlockedBridges;	
	// std::vector<int> suspectedBlockedBridgesRegionIndexes;

	Detector(BridgeAuthority* _bridgeAuth, int _reportThreshold, int _bridgeStatDiffThreshold, int _numDaysHistoricalBridgeStatsAvg, Random64* _rng) {
		rng = _rng;
		numDaysHistoricalBridgeStatsAvg = _numDaysHistoricalBridgeStatsAvg;
		bridgeAuth = _bridgeAuth;
		reportThreshold = _reportThreshold;
		bridgeStatDiffThreshold = _bridgeStatDiffThreshold;
		for (int i = 0; i < regionList.size(); i++) {
			probesPerRegionIndex[i] = new Probe(i);
		}
		// unresolvedReportPairs = nullptr;		
	}

	bool didProbeFailToAccessBridgeFromRegionIndex(Bridge* b, int regionIndex) {
		int retriesPerProbe = 10;
		bool probeReachedBridge = false;
		for (int i = 0; i < retriesPerProbe; i++) {
			probeReachedBridge = probesPerRegionIndex[regionIndex]->probeBridge(b);
			if (probeReachedBridge) {
				break;
			}
		}
		return !probeReachedBridge;
	}

	// Generic update function to perform tasks per time interval of main
	// update loop 
	void update() {
		int numNewBlocks = 0;

		double reportWeight = 0.4;
		double bridgeStatsDiffWeight = 0.6;
		double minConfidenceToProbe = 0.9;
		int bridgeUsageThreshold = 32;
		std::map<Bridge*, UserStackNode*>::iterator it;
		std::vector<Bridge*> suspectedBlockedBridges;	
		std::vector<int> suspectedBlockedBridgesRegionIndexes;

		// printf("%ld\n", bridgeAuth->bridgeUsersMap.size());
		for (it = bridgeAuth->bridgeUsersMap.begin(); it != bridgeAuth->bridgeUsersMap.end(); it++) {						
			Bridge* b = it->first;
			if (std::find(blockedBridges.begin(), blockedBridges.end(), b) != blockedBridges.end()) {
				continue;
			}

			if (std::find(suspectedBlockedBridges.begin(), suspectedBlockedBridges.end(), b) != suspectedBlockedBridges.end()) {
				continue;
			}

			for (int i = 0; i < censorRegionIndexList.size(); i++) {
				int censoredRegionIndex = censorRegionIndexList[i];
				double blockedConfidence = 0;
				double reportWeight = 0.2;
								
				int numReportsFromRegion = numReportsPerBridgeRegionIndex[censoredRegionIndex].find(b) == numReportsPerBridgeRegionIndex[censoredRegionIndex].end() 
				  ? 0 : numReportsPerBridgeRegionIndex[censoredRegionIndex][b];
				
				double reportConfidence = numReportsFromRegion >= reportThreshold ? reportThreshold : numReportsFromRegion;
				reportConfidence = reportWeight * (reportConfidence / reportThreshold);
				
				int bridgeStatsDiffFromAvg = bridgeStatsHistoricalAvgDiff(b, censoredRegionIndex);	
				double bStatsConfidence =  bridgeStatsDiffWeight * (bridgeStatsDiffFromAvg / bridgeStatDiffThreshold);

				int currentBridgeStats = b->getCurrentDailyUsageFromRegionIndex(censoredRegionIndex);
				if (currentBridgeStats < bridgeUsageThreshold) {
					bStatsConfidence = 1 * bridgeStatsDiffWeight;
				}

				blockedConfidence = reportConfidence + bStatsConfidence;
				// blockedConfidence = numReportsFromRegion >= reportThreshold && (currentBridgeStats < bridgeUsageThreshold || bridgeStatsDiffFromAvg >= bridgeStatDiffThreshold);

				if (blockedConfidence >= minConfidenceToProbe) {
					suspectedBlockedBridges.push_back(b);
					suspectedBlockedBridgesRegionIndexes.push_back(censoredRegionIndex);
					// if (didProbeFailToAccessBridgeFromRegionIndex(b, censoredRegionIndex)) {
					// 	blockedBridges.push_back(b);
					// 	numNewBlocks++;
					// 	launchedProbes++;						
					// 	break;
					// }
					break;
				}

				double r = rng->next(100000000) / 1000000.0;	
				double randomProbeChancePercent = 5;
				if (r < randomProbeChancePercent) {
					suspectedBlockedBridges.push_back(b);
					suspectedBlockedBridgesRegionIndexes.push_back(censoredRegionIndex);
				}
			}
		}

		double probeChancePercent = 90;		
		// double probeChancePercent = 101;		
		// printf("%ld\n", suspectedBlockedBridges.size());
		for (int i = 0; i < suspectedBlockedBridges.size(); i++) {
			double r = rng->next(100000000) / 1000000.0;	
			if (r < probeChancePercent) {				
				launchedProbes++;										
				if (didProbeFailToAccessBridgeFromRegionIndex(suspectedBlockedBridges[i], suspectedBlockedBridgesRegionIndexes[i])) {
					blockedBridges.push_back(suspectedBlockedBridges[i]);
					numNewBlocks++;
				}
			}	
		}
 
		for (int i = 0; i < blockedBridges.size(); i++) {			
			// std::vector<Bridge*>::iterator it = std::find(suspectedBlockedBridges.begin(), suspectedBlockedBridges.end(), blockedBridges[i]);
			// int index = it - suspectedBlockedBridges.begin();
			// suspectedBlockedBridges.erase(it);
			// suspectedBlockedBridgesRegionIndexes.erase(suspectedBlockedBridgesRegionIndexes.begin() + index);
			bridgeAuth->migrateAllUsersOffBridge(blockedBridges[i]);			
		}

		blockedBridgeDetectionCount += numNewBlocks;
#ifdef DEBUG2
		printf("Num blocked bridges = %ld\n", blockedBridgeDetectionCount);		
#endif
	}

	void reportBridgeFromRegionIndex(Bridge* bridge, int regionIndex) {
		// ReportPair* rp = new ReportPair(bridge, regionIndex);		
		// unresolvedReportPairs = new ReportPairStackNode(unresolvedReportPairs, rp);				
		// if (numReportsPerBridgeRegion.find(rp) != numReportsPerBridgeRegion.end()) {
		// 	numReportsPerBridgeRegion[rp]++;
		// }
		// else {
		// 	numReportsPerBridgeRegion[rp] = 1;
		// }


		std::map<Bridge*, int>* map = &numReportsPerBridgeRegionIndex[regionIndex];
		if (map->find(bridge) != map->end()) {
			(*map)[bridge]++;
		}
		else {
			(*map)[bridge] = 1;
		}
	}

	long getDetectedBlockagesCount() {
		return blockedBridgeDetectionCount;
	}

	// void clearUnresolvedReportPairs() {
	// 	ReportPairStackNode* top = unresolvedReportPairs;
	// 	while (top) {
	// 		delete top->rp;
	// 		ReportPairStackNode* old = top;
	// 		top = top->next;
	// 		delete old;
	// 	} 
	// 	unresolvedReportPairs = nullptr;
	// }
};