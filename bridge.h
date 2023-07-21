#pragma once

#include <map>
#include <stdlib.h>
#include <vector>
#include <string>
#include "xoshiro_srand.h"

#define BLOCKED_ACCESS -1
#define DROPPED_ACCESS -2

extern std::vector<std::string> regionList;

class Bridge {
private:		
	std::map<int, bool> perRegionIndexBlockage;
	double geoIPErrorChance;
	double messageDropChance;
	Random64* rng;
	std::vector<std::map<int, int>> dailyUsagePerRegionIndexHistory;
	std::map<int, int> currentDailyUsagePerRegionIndex;
	
public:
	Bridge(double _geoIPErroChance, double _messageDropChance, Random64* _rng) {
		messageDropChance = _messageDropChance;
		geoIPErrorChance = _geoIPErroChance;
		rng = _rng;
		for (int i = 0; i < regionList.size(); i++) {
			currentDailyUsagePerRegionIndex[i] = 0;
		}
		dailyUsagePerRegionIndexHistory.push_back(currentDailyUsagePerRegionIndex);
		currentDailyUsagePerRegionIndex.clear();
	}

	int messageFromRegion(int trueRegionIndex) {		
		//bridge is blocked from the senders region
		std::string region = regionList[trueRegionIndex];
		if (!perRegionIndexBlockage.empty()) {
			if (perRegionIndexBlockage.contains(trueRegionIndex)) {
				if (perRegionIndexBlockage[trueRegionIndex]) {
					return BLOCKED_ACCESS;
				}
			}
		}

		//message looks like the bridge is blocked when it really is not
		double r = rng->next(100000000) / 1000000.0;		
		if (r < messageDropChance) {
			return DROPPED_ACCESS;
		}

		int geoIPRegionIndex = trueRegionIndex;
		r = rng->next(100000000) / 1000000.0;		
		if (r < geoIPErrorChance) {
			geoIPRegionIndex = (geoIPRegionIndex + 1 + rng->next(100)) % regionList.size();
		}
		

		if (currentDailyUsagePerRegionIndex.contains(geoIPRegionIndex)) {
			currentDailyUsagePerRegionIndex[geoIPRegionIndex]++;
		}
		else {
			currentDailyUsagePerRegionIndex[geoIPRegionIndex] = 1;
		}

		return (rand() % 100) + 1;
	}

	bool blockFromRegion(int regionIndex) {
		bool wasUnblocked = true;
		if (perRegionIndexBlockage.contains(regionIndex)) {
			wasUnblocked = !perRegionIndexBlockage[regionIndex];
		}		
		perRegionIndexBlockage[regionIndex] = true;		
		return wasUnblocked;
	}

	void progressToNextDay() {
		//ciel to nearest multiple of 8
		for (int i = 0; i < regionList.size(); i++) {			
			currentDailyUsagePerRegionIndex[i] = ((currentDailyUsagePerRegionIndex[i] + 7) & (-8));
		}

		dailyUsagePerRegionIndexHistory.push_back(currentDailyUsagePerRegionIndex);
		currentDailyUsagePerRegionIndex.clear();
	}

	int getCurrentDailyUsageFromRegionIndex(int regionIndex) {
		//ciel to nearest multiple of 8
		return ((currentDailyUsagePerRegionIndex[regionIndex] + 7) & (-8));		
	}

	int getHistoricalDailyUsageFromRegionIndex(int daysPrior, int regionIndex) {
#ifdef DEBUG_SANITY
		if (daysPrior <= 0) {
			printf("ERROR: attempting to get historical bridge stats with 0 or negative days in the past\n");	
			exit(-1);
		}
#endif		
		
		int previousDateIndex = dailyUsagePerRegionIndexHistory.size() - daysPrior;		
		if (previousDateIndex <= dailyUsagePerRegionIndexHistory.size() - 1) {
			if (dailyUsagePerRegionIndexHistory[previousDateIndex].find(regionIndex) != dailyUsagePerRegionIndexHistory[previousDateIndex].end()){
				//ciel to nearest multiple of 8
				//this should be redundant since we did it in progressToNextDay() but going to leave it incase we change progressToNextDay later
				return ((dailyUsagePerRegionIndexHistory[previousDateIndex][regionIndex] + 7) & (-8));
			}
		}
		return 0;
	}

	bool isBlockedFromRegionIndex(int regionIndex) {
		return perRegionIndexBlockage[regionIndex];
	}
};