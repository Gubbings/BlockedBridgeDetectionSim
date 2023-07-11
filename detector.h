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

struct reportPair {
	Bridge* bridge;
	int regionIndex;

	bool operator=(const reportPair &other) const {
        return bridge == other.bridge && regionIndex == other.regionIndex;
    }

    bool operator<(const reportPair &other) const {
        return regionIndex < other.regionIndex;
    }
};

class Detector {
private:
	BridgeAuthority* bridgeAuth;
	Censor* censor;
	int reportThreshold;
	int blockedBridgeDetectionCount = 0;
	int bridgeStatDiffThreshold;

	long bridgeStatsDailyDiff(Bridge* b, int regionIndex) {
		int currUsage = b->getCurrentDailyUsageFromRegionIndex(regionIndex);
		int prevUsage = b->getHistoricalDailyUsageFromRegionIndex(1, regionIndex);				
		return prevUsage - currUsage;
	}

public:
	std::map<int, Probe*> probesPerRegionIndex;
	std::vector<reportPair> unresolvedReportPairs;
	std::map<reportPair, int> numReportsPerBridgeRegion;	

	Detector(BridgeAuthority* _bridgeAuth, Censor* _censor, int _reportThreshold, int _bridgeStatDiffThreshold) {		
		bridgeAuth = _bridgeAuth;
		censor = _censor;
		reportThreshold = _reportThreshold;
		bridgeStatDiffThreshold = _bridgeStatDiffThreshold;
		for (int i = 0; i < regionList.size(); i++) {
			probesPerRegionIndex[i] = new Probe(i, censor);
		}
	}

	bool probeBridgeFromRegionIndex(Bridge* b, int regionIndex) {
		return probesPerRegionIndex[regionIndex]->probeBridge(b);
	}

	// Generic update function to perform tasks per time interval of main
	// update loop 
	void update() {
		for (int i = 0; i < unresolvedReportPairs.size(); i++) {
			reportPair rp = unresolvedReportPairs[i];			
			if (numReportsPerBridgeRegion[rp] > reportThreshold) {
				//check bridge stats
				if (bridgeStatsDailyDiff(rp.bridge, rp.regionIndex) >= bridgeStatDiffThreshold) {
					//launch probe
					if (probeBridgeFromRegionIndex(rp.bridge, rp.regionIndex)) {
						blockedBridgeDetectionCount++;
						bridgeAuth->migrateAllUsersOffBridge(rp.bridge);
					}
				}				
			}
		}
	}

	void reportBridgeFromRegionIndex(Bridge* bridge, int regionIndex) {
		reportPair rp = {bridge, regionIndex};
		unresolvedReportPairs.push_back(rp);
		if (numReportsPerBridgeRegion.find(rp) != numReportsPerBridgeRegion.end()) {
			numReportsPerBridgeRegion[rp]++;
		}
		else {
			numReportsPerBridgeRegion[rp] = 1;
		}
	}
};