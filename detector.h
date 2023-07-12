#pragma once

#include "probe.h"
#include "censor.h"
#include "bridge.h"
#include <map>
#include <vector>
#include <string>
#include "bridgeAuthority.fwd.h"
// #include <utility> 

extern std::vector<std::string> regionList;

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

	ReportPair(ReportPair &other) {
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
	Censor* censor;
	int reportThreshold;
	long blockedBridgeDetectionCount = 0;
	int bridgeStatDiffThreshold;

	long bridgeStatsDailyDiff(Bridge* b, int regionIndex) {
		int currUsage = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		int prevUsage = b->getHistoricalDailyUsageFromRegionIndex(1, regionIndex);				
		return prevUsage - currUsage;
	}

public:
	std::map<int, Probe*> probesPerRegionIndex;
	// std::vector<ReportPair*> unresolvedReportPairs;
	ReportPairStackNode* unresolvedReportPairs;
	std::map<ReportPair*, int> numReportsPerBridgeRegion;	
	std::vector<Bridge*> blockedBridges;


	Detector(BridgeAuthority* _bridgeAuth, Censor* _censor, int _reportThreshold, int _bridgeStatDiffThreshold) {		
		bridgeAuth = _bridgeAuth;
		censor = _censor;
		reportThreshold = _reportThreshold;
		bridgeStatDiffThreshold = _bridgeStatDiffThreshold;
		for (int i = 0; i < regionList.size(); i++) {
			probesPerRegionIndex[i] = new Probe(i, censor);
		}
		unresolvedReportPairs = nullptr;
	}

	bool probeBridgeFromRegionIndex(Bridge* b, int regionIndex) {
		return !probesPerRegionIndex[regionIndex]->probeBridge(b);
	}

	// Generic update function to perform tasks per time interval of main
	// update loop 
	void update() {
		int numNewBlocks = 0;

		ReportPairStackNode* top = unresolvedReportPairs;
		while (top) {
			ReportPair* rp = top->rp;
			if (find(blockedBridges.begin(), blockedBridges.end(), rp->bridge) == blockedBridges.end()) {		
				if (numReportsPerBridgeRegion[rp] > reportThreshold) {
					//check bridge stats
					if (bridgeStatsDailyDiff(rp->bridge, rp->regionIndex) >= bridgeStatDiffThreshold) {
						//launch probe
						if (probeBridgeFromRegionIndex(rp->bridge, rp->regionIndex)) {
							blockedBridges.push_back(rp->bridge);
							numNewBlocks++;
							bridgeAuth->migrateAllUsersOffBridge(rp->bridge);
						}
					}
				}
			}	
			top = top->next;
		}

		// for (int i = 0; i < unresolvedReportPairs.size(); i++) {
		// 	ReportPair* rp = unresolvedReportPairs[i];	
		// 	if (std::find(blockedBridges.begin(), blockedBridges.end(), rp->bridge) == blockedBridges.end()) {		
		// 		if (numReportsPerBridgeRegion[rp] > reportThreshold) {
		// 			//check bridge stats
		// 			if (bridgeStatsDailyDiff(rp->bridge, rp->regionIndex) >= bridgeStatDiffThreshold) {
		// 				//launch probe
		// 				if (probeBridgeFromRegionIndex(rp->bridge, rp->regionIndex)) {
		// 					blockedBridges.push_back(rp->bridge);
		// 					numNewBlocks++;
		// 					bridgeAuth->migrateAllUsersOffBridge(rp->bridge);
		// 				}
		// 			}
		// 		}
		// 	}			
		// }
		blockedBridgeDetectionCount += numNewBlocks;
		// blockedBridgeDetectionCount += blockedBridges.size();
		// blockedBridges.clear();
		// unresolvedReportPairs.clear();
		clearUnresolvedReportPairs();
	}

	void reportBridgeFromRegionIndex(Bridge* bridge, int regionIndex) {
		ReportPair* rp = new ReportPair(bridge, regionIndex);
		// unresolvedReportPairs.push_back(rp);
		unresolvedReportPairs = new ReportPairStackNode(unresolvedReportPairs, rp);		
		
		if (numReportsPerBridgeRegion.find(rp) != numReportsPerBridgeRegion.end()) {
			numReportsPerBridgeRegion[rp]++;
		}
		else {
			numReportsPerBridgeRegion[rp] = 1;
		}
	}

	long getDetectedBlockagesCount() {
		return blockedBridgeDetectionCount;
	}

	void clearUnresolvedReportPairs() {
		ReportPairStackNode* top = unresolvedReportPairs;
		while (top) {
			delete top->rp;
			ReportPairStackNode* old = top;
			top = top->next;
			delete old;
		} 
		unresolvedReportPairs = nullptr;
	}
};