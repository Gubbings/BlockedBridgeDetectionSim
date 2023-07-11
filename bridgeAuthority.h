#pragma once

#include "bridgeAuthority.fwd.h"
#include "user.fwd.h"
#include "bridge.h"
#include <vector>
#include <stdlib.h>    
#include <map>
#include <assert.h>
#include <algorithm> 
#include "xoshiro_srand.h"   

#define MIN_BRIDGE_DB_SIZE 10


class BridgeAuthority {
private:
	double geoIPErrorChance;
	Random64* rng;

	//Map users to the bridges that they currently known about and access	
	std::map<User*, Bridge*> userBridgeMap;	

	//Map bridges to the users that currently have access to them
	std::map<Bridge*, std::vector<User*>> bridgeUsersMap;
	
	//list of all (unblocked) open entry bridges
	//we could seperately have invite only if we want to model that
	std::vector<Bridge*> bridgeDB;	

	void expandBridgeDB(int numNewBridges) {
		for (int i = 0; i < numNewBridges; i++) {
			Bridge* b = new Bridge(geoIPErrorChance, rng);
			bridgeDB.push_back(b);
		}
	}

	void discardBridge(Bridge* b) {
		bridgeDB.erase(std::find(bridgeDB.begin(), bridgeDB.end(), b));
		bridgeUsersMap.erase(b);
	}


	//todo
	Bridge* getBridge() {		
		assert(!bridgeDB.empty());
		return bridgeDB[0];
	}


public:
	BridgeAuthority(int _initBridgeCount, double _geoIPErrorChance, Random64* rng) {
		geoIPErrorChance = _geoIPErrorChance;
		bridgeDB.reserve(_initBridgeCount);
		expandBridgeDB(_initBridgeCount);		
	}

	Bridge* requestNewBridge(User* user) {
		if (userBridgeMap.find(user) != userBridgeMap.end()) {
			return migrateToNewBridge(user);
		}
		else {
			Bridge* b = getBridge();
			userBridgeMap[user] = b;
			bridgeUsersMap[b].push_back(user);
			return b;
		}
	}
	

	Bridge* migrateToNewBridge(User* user) {
		Bridge* oldBridge = userBridgeMap[user];		
		discardBridge (oldBridge);				

		if (bridgeDB.size() <= 0) {
			expandBridgeDB(MIN_BRIDGE_DB_SIZE);
		}
		
		Bridge* newBridge = getBridge();
		userBridgeMap[user] = newBridge;
		bridgeUsersMap[newBridge].push_back(user);
		return newBridge;
	}

	void publishDailyBridgeStats() {
		for(int i = 0; i < bridgeDB.size(); i++) {
			bridgeDB[i]->progressToNextDay();
		}
	}

	void migrateAllUsersOffBridge(Bridge* b) {
		for (int i = 0; i < bridgeUsersMap[b].size(); i++) {
			migrateToNewBridge(bridgeUsersMap[b][i]);
		}				
	}
};