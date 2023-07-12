#pragma once

#include "bridge.h"
#include "xoshiro_srand.h"
#include <vector>

class Censor {
	Random64* rng;
	double blockChance;
	long blockedBridgeCount = 0;
public:
	std::vector<Bridge*> knownBridges;	

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

		double r = rng->next(100000000) / 1000000.0;		
		if (r < blockChance) {
			blockBridge(b);
		}
	}

	void blockBridge(Bridge* b) {
		if (b->blockFromRegion(regionIndex)) {
			knownBridges.push_back(b);
			blockedBridgeCount++;
		}
	}

	long getBlockedBridgeCount() {
		return blockedBridgeCount;
	}

	// Generic update function to perform tasks per time interval of main
	// update loop
	void update() {

	}
};