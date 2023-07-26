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
#include <chrono>



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

	uint64_t launchedProbes = 0;
	uint64_t blockedBridgeDetectionCount = 0;
	uint64_t bstatsUsageBelowThresholdCount = 0;
	uint64_t totalSuspiciousBridges = 0;

	int reportThreshold;
	int numDaysHistoricalBridgeStatsAvg;

	double probeChancePercent;		
	double reportWeight;
	double bridgeStatsDiffWeight;
	double minConfidenceToProbe;
	double nonSusProbeChancePercent;
	int bridgeUsageThreshold;
	int retriesPerProbe;

	uint64_t durationTotal = 0;
	uint64_t durationCount = 0;


	long bridgeStatsDailyDiff(Bridge* b, int regionIndex) {
		int currUsageMultipleOf8 = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		int prevUsageMultipleOf8 = b->getHistoricalDailyUsageFromRegionIndex(1, regionIndex);				
		return prevUsageMultipleOf8 - currUsageMultipleOf8;
	}

	long bridgeStatsHistoricalAvg(Bridge* b, int regionIndex) {
		long totalHistoricalUsage = 0;
		for (int i = 1; i <= numDaysHistoricalBridgeStatsAvg; i++) {
			totalHistoricalUsage += b->getHistoricalDailyUsageFromRegionIndex(i, regionIndex);
		}
		long historicalAverage = ceil(totalHistoricalUsage / (numDaysHistoricalBridgeStatsAvg * 1.0));
		long historicalAverageMultipleOf8 = (historicalAverage + 7) & (-8);
		return historicalAverageMultipleOf8;
	}

	long bridgeStatsHistoricalAvgDiff(Bridge* b, int regionIndex) {		
		long historicalAverageMultipleOf8 = bridgeStatsHistoricalAvg(b, regionIndex);
		long currUsageMultipleOf8 = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		return historicalAverageMultipleOf8 - currUsageMultipleOf8;
	}

	long bridgeStatsHistoricalAvgDiff(Bridge* b, int regionIndex, long historicalAverageMultipleOf8) {				
		long currUsageMultipleOf8 = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		return historicalAverageMultipleOf8 - currUsageMultipleOf8;
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

	uint64_t step1TruePos = 0;
	uint64_t step1FalsePos = 0;
	uint64_t step1TrueNeg = 0;
	uint64_t step1FalseNeg = 0;

	Detector(BridgeAuthority* _bridgeAuth, int _reportThreshold, int _numDaysHistoricalBridgeStatsAvg, 
	  double _probeChancePercent, double _reportWeight, double _bridgeStatsDiffWeight, double _minConfidenceToProbe,
	  int _bridgeUsageThreshold, int _retriesPerProbe, double _nonSusProbeChancePercent, Random64* _rng) {
		rng = _rng;		
		numDaysHistoricalBridgeStatsAvg = _numDaysHistoricalBridgeStatsAvg;
		bridgeAuth = _bridgeAuth;
		reportThreshold = _reportThreshold;
		retriesPerProbe = _retriesPerProbe;
		nonSusProbeChancePercent = _nonSusProbeChancePercent;

		probeChancePercent = _probeChancePercent;
		reportWeight = _reportWeight;
		bridgeStatsDiffWeight = _bridgeStatsDiffWeight;
		minConfidenceToProbe = _minConfidenceToProbe;
		bridgeUsageThreshold = _bridgeUsageThreshold;

		for (int i = 0; i < regionList.size(); i++) {
			probesPerRegionIndex[i] = new Probe(i);
		}
		// unresolvedReportPairs = nullptr;		
	}

	bool didProbeFailToAccessBridgeFromRegionIndex(Bridge* b, int regionIndex) {
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
		durationCount++;
		std::chrono::high_resolution_clock::time_point t1, t2;
		t1 = std::chrono::high_resolution_clock::now(); 

		int numNewBlocks = 0;
		
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
				
				double r = rng->next(100000000) / 1000000.0;	
				if (r < nonSusProbeChancePercent) {
					suspectedBlockedBridges.push_back(b);
					suspectedBlockedBridgesRegionIndexes.push_back(censoredRegionIndex);
					continue;					
				}
				
				double blockedConfidence = 0;
				double reportWeight = 0.2;

				int numReportsFromRegion = 0;				
				if (numReportsPerBridgeRegionIndex[censoredRegionIndex].find(b) != numReportsPerBridgeRegionIndex[censoredRegionIndex].end()) {
					numReportsFromRegion = numReportsPerBridgeRegionIndex[censoredRegionIndex][b];
					numReportsPerBridgeRegionIndex[censoredRegionIndex][b] = 0;
				}				
								
				double reportConfidence = numReportsFromRegion >= reportThreshold ? reportThreshold : numReportsFromRegion;
				reportConfidence = reportWeight * std::min(1.0, (reportConfidence / reportThreshold));
				
				// reportConfidence = reportWeight * (reportConfidence / reportThreshold);

				long bStatsHistoricalAvg = bridgeStatsHistoricalAvg(b, censoredRegionIndex);
				long bridgeStatsDiffFromAvg = bridgeStatsHistoricalAvgDiff(b, censoredRegionIndex, bStatsHistoricalAvg);					
				double bStatsConfidence =  bridgeStatsDiffWeight * std::min(1.0, std::max(0.0, (bridgeStatsDiffFromAvg / (bStatsHistoricalAvg * 1.0))));
				
#ifdef DEBUG3
				printf("bridge=%p, ", b);
				printf("reps=%d, ", numReportsFromRegion);
				printf("bstat-avg=%ld, ", bStatsHistoricalAvg);
				printf("bstat-curr=%d, ", b->getCurrentDailyUsageFromRegionIndex(censoredRegionIndex));
				printf("bstat-diff=%ld, ", bridgeStatsDiffFromAvg);
				printf("r=%f, ", reportConfidence);
				printf("b1=%f, ", bStatsConfidence);
#endif
				if (bStatsHistoricalAvg > bridgeUsageThreshold) {
					int currentBridgeStats = b->getCurrentDailyUsageFromRegionIndex(censoredRegionIndex);
					if (currentBridgeStats < bridgeUsageThreshold) {
						bStatsConfidence = 1 * bridgeStatsDiffWeight;
						bstatsUsageBelowThresholdCount++;
					}
				}

				// if (bStatsHistoricalAvg > 1) {
				// 	int currentBridgeStats = b->getCurrentDailyUsageFromRegionIndex(censoredRegionIndex);
				// 	if (currentBridgeStats == 0) {
				// 		bStatsConfidence = 1 * bridgeStatsDiffWeight;
				// 		bstatsUsageBelowThresholdCount++;
				// 	}
				// }

				blockedConfidence = reportConfidence + bStatsConfidence;

#ifdef DEBUG3
				printf("b2=%f, ", bStatsConfidence);
				printf("t=%f, ", blockedConfidence);
#endif				
#ifdef DEBUG1				
				printf("diff:  %d\n", bridgeStatsDiffFromAvg);
				printf("daily: %d\n", currentBridgeStats);
#endif				
				if (blockedConfidence >= minConfidenceToProbe) {
					suspectedBlockedBridges.push_back(b);
					suspectedBlockedBridgesRegionIndexes.push_back(censoredRegionIndex);
					//bridge is blocked and we put it in sus bridges
					if (b->isBlockedFromRegionIndex(censoredRegionIndex)) {
						step1TruePos++;
#ifdef DEBUG3						
						printf("true pos\n");
#endif						
					}
					else {
						//bridge is not blocked and we put it in sus bridges
						step1FalsePos++;
#ifdef DEBUG3						
						printf("false pos\n");
#endif						
					}					
					continue;					
				}

				//bridge is blocked and we didnt put it in sus bridges
				if (b->isBlockedFromRegionIndex(censoredRegionIndex)) {
					step1FalseNeg++;
#ifdef DEBUG3					
					printf("false neg\n");
#endif
				}
				else {
					//bridge is not blocked and we didnt put it in sus bridges
					step1TrueNeg++;
#ifdef DEBUG3					
					printf("true neg\n");
#endif					
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
		totalSuspiciousBridges += suspectedBlockedBridges.size();

		for (int i = 0; i < blockedBridges.size(); i++) {			
			// std::vector<Bridge*>::iterator it = std::find(suspectedBlockedBridges.begin(), suspectedBlockedBridges.end(), blockedBridges[i]);
			// int index = it - suspectedBlockedBridges.begin();
			// suspectedBlockedBridges.erase(it);
			// suspectedBlockedBridgesRegionIndexes.erase(suspectedBlockedBridgesRegionIndexes.begin() + index);
			bridgeAuth->migrateAllUsersOffBridge(blockedBridges[i]);			
		}

		blockedBridgeDetectionCount += numNewBlocks;
		blockedBridges.clear();
#ifdef DEBUG2
		printf("Num blocked bridges = %ld\n", blockedBridgeDetectionCount);		
#endif

		t2 = std::chrono::high_resolution_clock::now();
		durationTotal += std::chrono::duration_cast<std::chrono::nanoseconds>( t2 - t1 ).count();;
	}

	void reportBridgeFromRegionIndex(Bridge* bridge, int regionIndex) {		
		std::map<Bridge*, int>* map = &numReportsPerBridgeRegionIndex[regionIndex];
		if (map->contains(bridge)) {
			(*map)[bridge]++;
		}
		else {
			(*map)[bridge] = 1;
		}
	}

	uint64_t getDetectedBlockagesCount() {
		return blockedBridgeDetectionCount;
	}

	uint64_t getNumLaunchedProbes() {
		return launchedProbes;
	}

	uint64_t getBstatsUsageBelowThresholdCount() {
		return bstatsUsageBelowThresholdCount;
	}

	uint64_t getTotalSuspiciousBridgesCount() {
		return totalSuspiciousBridges;
	}

	void printAverageRuntimeOfUpdate() {
		double avg = (durationTotal / durationCount * 1.0);
		printf("average_nanoseconds_of_detector_update=%f\n", avg);
	}

	void printAverageProbesLaunchedPerUpdate() {
		double avg = (launchedProbes / durationCount * 1.0);
		printf("average_probes_per_detector_update=%f\n", avg);
	}
};