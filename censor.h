#pragma once

#include "bridge.h"
#include "xoshiro_srand.h"
#include <vector>

#ifdef DEBUG_SANITY		
	#include <set>
#endif

class Censor {
	Random64* rng;
	double blockChance;
	long blockedBridgeCount = 0;
public:
	std::vector<Bridge*> knownBridges;	
	int numBridgeAccessesFromCensoredRegion = 0;
	int regionIndex;

	Censor(int _regionIndex, double _blockChance, Random64* _rng) {
		regionIndex = _regionIndex;
		rng = _rng;
		blockChance = _blockChance;
	}

	void bridgeAccessFromRegionIndex(int fromRegionIndex, Bridge* b) {
		if (fromRegionIndex != regionIndex) {
			return;
		}

		numBridgeAccessesFromCensoredRegion++;

		double r = rng->next(100000000) / 1000000.0;		
		if (r < blockChance) {
			blockBridge(b);
		}
	}

	void blockBridge(Bridge* b) {		
		if (b->blockFromRegion(regionIndex)) {
#ifdef DEBUG_SANITY
			if (std::find(knownBridges.begin(), knownBridges.end(), b) != knownBridges.end()) {
				printf("problem\n");
			}
#endif
			knownBridges.push_back(b);
			blockedBridgeCount++;
		}
	}

	long getBlockedBridgeCount() {
#ifdef DEBUG_SANITY		
		std::set<Bridge*> s;
		for (int i = 0; i < knownBridges.size(); i++) {
			s.insert(knownBridges[i]);
		} 
		printf("%ld\n", s.size());
		printf("\n");
#endif
		return blockedBridgeCount;
	}

	// Generic update function to perform tasks per time interval of main
	// update loop
	void update() {
#ifdef DEBUG2
		printf("Num censored bridges = %ld\n", blockedBridgeCount);
#endif
	}
};