#pragma once

#include "user.fwd.h"
#include "bridgeAuthority.fwd.h"
#include <string>
#include <vector>
#include "bridge.h"
#include "detector.h"
#include "xoshiro_srand.h"
#include "censor.h"

class User {
private:
	uint64_t numReports = 0;
	int maxAccessesPerUpdate;
	int minAccesesPerUpdate;
	double reportChance;
	Random64* rng;
	Censor* censor;

public:	
	Bridge* currentAccessPoint = nullptr;
	BridgeAuthority* bridgeAuth;
	Detector* detector;
	int regionIndex;

	User(){
		censor = nullptr;
		rng = nullptr;
		reportChance = -1;
		bridgeAuth = nullptr;
		detector = nullptr;
		maxAccessesPerUpdate = -1;
		minAccesesPerUpdate = -1;
	}

	User(BridgeAuthority* _bridgeAuth, Detector* _detector, Censor* _censor, 
	  int _maxAccessesPerUpdate, int _minAccesesPerUpdate, 
	  double _reportChance, Random64* _rng) {
		censor = _censor;
		rng = _rng;
		reportChance = _reportChance;
		bridgeAuth = _bridgeAuth;
		detector = _detector;
		maxAccessesPerUpdate = _maxAccessesPerUpdate;
		minAccesesPerUpdate = _minAccesesPerUpdate;
	}

	User(User &other) {
		censor = other.censor;
		rng = other.rng;
		reportChance = other.reportChance;
		bridgeAuth = other.bridgeAuth;
		detector = other.detector;
		maxAccessesPerUpdate = other.maxAccessesPerUpdate;
		minAccesesPerUpdate = other.minAccesesPerUpdate;
	}

	~User() {
	}

	void reportBridge() {
		detector->reportBridgeFromRegionIndex(currentAccessPoint, regionIndex);
	}

	void accessBridge(double reportChance, Random64* rng) {
		int response = currentAccessPoint->messageFromRegion(regionIndex);
		censor->bridgeAccessFromRegionIndex(regionIndex, currentAccessPoint);
		if (response < 0) {
			double r = rng->next(100000000) / 1000000.0;
			if (r < reportChance) {
				reportBridge();
				numReports++;
			}
		}
	}

	// Generic update function to perform tasks per time interval of main
	// update loop
	void update() {
		if (!currentAccessPoint) {
			currentAccessPoint = bridgeAuth->requestNewBridge(this);
		}

		int bridgeAccesses = minAccesesPerUpdate + rng->next(maxAccessesPerUpdate);
		for (int i = 0; i < bridgeAccesses; i++) {
			accessBridge(reportChance, rng);
		}
	}
};