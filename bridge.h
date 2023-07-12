#pragma once

#include <map>
#include <stdlib.h>
#include <vector>
#include <string>
#include "xoshiro_srand.h"

extern std::vector<std::string> regionList;

class Bridge {
private:		
	std::map<int, bool> perRegionIndexBlockage;
	double geoIPErrorChance;
	Random64* rng;
	std::vector<std::map<int, int>> dailyUsagePerRegionIndexHistory;
	std::map<int, int> currentDailyUsagePerRegionIndex;
	
public:
	Bridge(double _geoIPErroChance, Random64* _rng) {
		geoIPErrorChance = _geoIPErroChance;
		rng = _rng;
		for (int i = 0; i < regionList.size(); i++) {
			currentDailyUsagePerRegionIndex[i] = 0;
		}
		dailyUsagePerRegionIndexHistory.push_back(currentDailyUsagePerRegionIndex);
		currentDailyUsagePerRegionIndex.clear();
	}

	int messageFromRegion(int trueRegionIndex) {
		std::string region = regionList[trueRegionIndex];
		if (!perRegionIndexBlockage.empty()) {
			if (perRegionIndexBlockage.find(trueRegionIndex) != perRegionIndexBlockage.end()) {
				if (perRegionIndexBlockage[trueRegionIndex]) {
					return -1;
				}
			}
		}

		int geoIPRegionIndex = trueRegionIndex;
		double r = rng->next(100000000) / 1000000.0;		
		if (r < geoIPErrorChance) {
			geoIPRegionIndex = (geoIPRegionIndex + 1 + rng->next(100)) % regionList.size();
		}
		

		if (currentDailyUsagePerRegionIndex.find(geoIPRegionIndex) != currentDailyUsagePerRegionIndex.end()) {
			currentDailyUsagePerRegionIndex[geoIPRegionIndex]++;
		}
		else {
			currentDailyUsagePerRegionIndex[geoIPRegionIndex] = 1;
		}

		return (rand() % 100) + 1;
	}

	bool blockFromRegion(int regionIndex) {
		bool wasUnblocked = true;
		if (perRegionIndexBlockage.find(regionIndex) != perRegionIndexBlockage.end()) {
			wasUnblocked = !perRegionIndexBlockage[regionIndex];
		}		
		perRegionIndexBlockage[regionIndex] = true;		
		return wasUnblocked;
	}

	void progressToNextDay() {
		dailyUsagePerRegionIndexHistory.push_back(currentDailyUsagePerRegionIndex);
		currentDailyUsagePerRegionIndex.clear();
	}

	int getCurrentDailyUsageFromRegionIndex(int regionIndex) {
		return currentDailyUsagePerRegionIndex[regionIndex];
	}

	int getHistoricalDailyUsageFromRegionIndex(int daysPrior, int regionIndex) {
		int yesterday = dailyUsagePerRegionIndexHistory.size() - 1;
		int previousDateIndex = yesterday - daysPrior - 1;
		if (previousDateIndex <= dailyUsagePerRegionIndexHistory.size() - 1) {
			if (dailyUsagePerRegionIndexHistory[previousDateIndex].find(regionIndex) != dailyUsagePerRegionIndexHistory[previousDateIndex].end()){
				return dailyUsagePerRegionIndexHistory[previousDateIndex][regionIndex];
			}
		}
		return 0;
	}
};