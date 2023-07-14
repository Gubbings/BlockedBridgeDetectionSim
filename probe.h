#pragma once

#include "bridge.h"
#include "censor.h"
#include <vector>

extern std::vector<Censor*> censors;

class Probe {
public:
	int regionIndex;

	Probe(int _regionIndex) {
		regionIndex = _regionIndex;
	}

	bool probeBridge(Bridge* b) {
		int response = b->messageFromRegion(regionIndex);

		for (int i = 0; i < censors.size(); i++) {
			censors[i]->bridgeAccessFromRegionIndex(regionIndex, b);
		}

		return response > 0;		
	}
};