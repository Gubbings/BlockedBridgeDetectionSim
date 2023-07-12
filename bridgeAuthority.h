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

struct UserStackNode {
	UserStackNode* next;
	User* user;

	UserStackNode() {
		user = nullptr;
		next = nullptr;
	}

	UserStackNode(UserStackNode* _next, User* _user) {
		user = _user;
		next = _next;
	}
};

class BridgeAuthority {
private:
	double geoIPErrorChance;
	Random64* rng;

	//Map users to the bridges that they currently known about and access	
	std::map<User*, Bridge*> userBridgeMap;	

	//Map bridges to the users that currently have access to them
	// std::map<Bridge*, std::vector<User*>> bridgeUsersMap;
	std::map<Bridge*, UserStackNode*> bridgeUsersMap;
	
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
		std::vector<Bridge*>::iterator it = std::find(bridgeDB.begin(), bridgeDB.end(), b);
		if (it != bridgeDB.end()) {
			bridgeDB.erase(it);
			bridgeUsersMap.erase(b);
		}				
	}


	//todo - fix this to be something more reasonable
	Bridge* getBridge() {		
		assert(!bridgeDB.empty());
		int bridgeIndex = rng->next(bridgeDB.size() - 1);
		return bridgeDB[bridgeIndex];
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
			// bridgeUsersMap[b].push_back(user);
			UserStackNode* oldTop = bridgeUsersMap.find(b) == bridgeUsersMap.end() ? nullptr : bridgeUsersMap[b]; 
			bridgeUsersMap[b] = new UserStackNode(oldTop, user);
			return b;
		}
	}
	

	Bridge* migrateToNewBridge(User* user) {
		Bridge* oldBridge = userBridgeMap[user];		
		discardBridge (oldBridge);				

		if (bridgeDB.size() <= MIN_BRIDGE_DB_SIZE) {
			expandBridgeDB(MIN_BRIDGE_DB_SIZE * 2);
		}
		
		Bridge* newBridge = getBridge();
		userBridgeMap[user] = newBridge;
		UserStackNode* oldTop = bridgeUsersMap.find(newBridge) == bridgeUsersMap.end() ? nullptr : bridgeUsersMap[newBridge]; 		
		bridgeUsersMap[newBridge] = new UserStackNode(oldTop, user);
		// bridgeUsersMap[newBridge].push_back(user);
		return newBridge;
	}

	void publishDailyBridgeStats() {
		for(int i = 0; i < bridgeDB.size(); i++) {
			bridgeDB[i]->progressToNextDay();
		}
	}

	void migrateAllUsersOffBridge(Bridge* b) {
		UserStackNode* top = bridgeUsersMap[b];
		while (top) {
			migrateToNewBridge(top->user);
			top = top->next;			
		}

		// for (int i = 0; i < bridgeUsersMap[b].size(); i++) {
		// 	migrateToNewBridge(bridgeUsersMap[b][i]);
		// }				
	}
};