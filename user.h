#pragma once

#include "user.fwd.h"
#include "bridgeAuthority.fwd.h"
#include <string>
#include <vector>
#include "bridge.h"
#include "detector.h"
#include "xoshiro_srand.h"
#include "censor.h"

extern std::vector<Censor*> censors;

class User {
private:
	int maxAccessesPerUpdate;
	int minAccesesPerUpdate;
	double reportChance;
	Random64* rng;
	int numKnownBridges = 0;
	int currentBridgeIndex = 0;

public:	
	Bridge* knownBridges[MAX_KNOWN_BRIDGES];

	BridgeAuthority* bridgeAuth;
	Detector* detector;
	int regionIndex;
	uint64_t numReports = 0;
	uint64_t numFailedBridgeAccesses = 0;
	uint64_t numBlockedBridgeAccesses = 0;
	uint64_t numDroppedBridgeAccesses = 0;
	uint64_t numBridgeAccesses = 0;
	uint64_t numSuccessfulBridgeAccesses = 0;


	User(){
		rng = nullptr;
		reportChance = -1;
		bridgeAuth = nullptr;
		detector = nullptr;
		maxAccessesPerUpdate = -1;
		minAccesesPerUpdate = -1;
	}

	User(BridgeAuthority* _bridgeAuth, Detector* _detector, 
	  int _maxAccessesPerUpdate, int _minAccesesPerUpdate, 
	  double _reportChance, Random64* _rng) {
		rng = _rng;
		reportChance = _reportChance;
		bridgeAuth = _bridgeAuth;
		detector = _detector;
		maxAccessesPerUpdate = _maxAccessesPerUpdate;
		minAccesesPerUpdate = _minAccesesPerUpdate;
	}

	User(User &other) {
		rng = other.rng;
		reportChance = other.reportChance;
		bridgeAuth = other.bridgeAuth;
		detector = other.detector;
		maxAccessesPerUpdate = other.maxAccessesPerUpdate;
		minAccesesPerUpdate = other.minAccesesPerUpdate;
	}

	~User() {
	}

	void reportBridge(Bridge* b) {
		detector->reportBridgeFromRegionIndex(b, regionIndex);
	}

	bool accessBridge() {
		numBridgeAccesses++;
		// int randomKnownBridgeIndex = rng->next(numKnownBridges);
		Bridge* b = knownBridges[currentBridgeIndex];

		int response = b->messageFromRegion(regionIndex);
		
		for (int i = 0; i < censors.size(); i++) {
			censors[i]->bridgeAccessFromRegionIndex(regionIndex, b);
		}

		if (response < 0) {
			double r = rng->next(100000000) / 1000000.0;
			if (r < reportChance) {
				reportBridge(b);
				numReports++;
			}
			numFailedBridgeAccesses++;

			if (response == BLOCKED_ACCESS) {
 				numBlockedBridgeAccesses++;
			}
			else if (response == DROPPED_ACCESS) {
				numDroppedBridgeAccesses++;
			}

			return false;
		}
		
		numSuccessfulBridgeAccesses++;
		return true;
	}

	// Generic update function to perform tasks per time interval of main
	// update loop
	void update() {
		if (numKnownBridges == 0) {
			numKnownBridges = bridgeAuth->requestNewBridge(this, knownBridges);
		}

		int bridgeAccesses = minAccesesPerUpdate + rng->next(maxAccessesPerUpdate);
		// int maxFails = 5;
		// int fails = 0;
		for (int i = 0; i < bridgeAccesses; i++) {
			if (!accessBridge()) {
				currentBridgeIndex = (currentBridgeIndex + 1) % numKnownBridges;
			}
			// if (!accessBridge()) {
			// 	fails++;
			// }
			// if (fails >= maxFails) {
			// 	detector->update();
			// }
		}
	}
	
};