#pragma once

#include "bridge.h"
#include "censor.h"

class Probe {
private:
	Censor* censor;
public:
	int regionIndex;

	Probe(int _regionIndex, Censor* _censor) {
		regionIndex = _regionIndex;
		censor = _censor;
	}

	bool probeBridge(Bridge* b) {
		if (b->messageFromRegion(regionIndex) > 0) {
			return true;
		}
		else {
			return false;
		}
	}
};