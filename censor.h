#pragma once

#include "bridge.h"
#include "xoshiro_srand.h"
#include <vector>

class Censor {
	std::vector<Bridge*> knownBridges;	
	Random64* rng;
	double blockChance;
	long blockedBridgeCount = 0;
public:
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
		b->blockFromRegion(regionIndex);
		knownBridges.push_back(b);
		blockedBridgeCount++;
	}

	// Generic update function to perform tasks per time interval of main
	// update loop
	void update() {

	}
};