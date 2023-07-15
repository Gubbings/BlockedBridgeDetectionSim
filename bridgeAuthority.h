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
	double messageDropChance;
	Random64* rng;
	int minBridgeDBSize = 1;

	//Map users to the bridges that they currently known about and can access	
	std::map<User*, Bridge**> userBridgeMap;	
	std::map<User*, int> userNumKnownBridgeMap;	
	
	//list of all bridges
	//we could seperately have invite only / open entry bridges if we want to model that
	std::vector<Bridge*> bridgeDB;	

	void expandBridgeDB(int numNewBridges) {
		for (int i = 0; i < numNewBridges; i++) {
			Bridge* b = new Bridge(geoIPErrorChance, messageDropChance, rng);
			bridgeDB.push_back(b);
		}
		numAddedBridges += numNewBridges;
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

	void migrateUserOffBridge(User* user, Bridge* b) {
		Bridge** userBridges = userBridgeMap[user];
		int numKnownBridges = userNumKnownBridgeMap[user];
		int oldBridgeIndex;
		for (oldBridgeIndex = 0; oldBridgeIndex < numKnownBridges; oldBridgeIndex++) {
			if (userBridges[oldBridgeIndex] == b) {
				break;
			}
		}

#ifdef DEBUG_SANITY
		if (oldBridgeIndex >= numKnownBridges) {
			printf("ERROR: attempting to migrate user off of bridge that the user does not know about\n");
			exit(-1);
		}
#endif
						
		Bridge* newBridge = getBridge();
		userBridgeMap[user][oldBridgeIndex] = newBridge;
		UserStackNode* oldTop = bridgeUsersMap.find(newBridge) == bridgeUsersMap.end() ? nullptr : bridgeUsersMap[newBridge]; 		
		bridgeUsersMap[newBridge] = new UserStackNode(oldTop, user);
		return;
	}

public:
	//Map bridges to the users that currently have access to them	
	std::map<Bridge*, UserStackNode*> bridgeUsersMap;

	int numAddedBridges = 0;

	BridgeAuthority(int _initBridgeCount, double _geoIPErrorChance, double _messageDropChance, int _minBridgeDBSize, Random64* rng) {
		minBridgeDBSize = _minBridgeDBSize;
		messageDropChance = _messageDropChance;
		geoIPErrorChance = _geoIPErrorChance;
		bridgeDB.reserve(_initBridgeCount);
		expandBridgeDB(_initBridgeCount);		
		numAddedBridges = 0;
	}

	int requestNewBridge(User* user, Bridge** userBridges) {
#ifdef DEBUG_SANITY
		if (userBridgeMap.contains(user)) {			
			printf("ERROR: user is requesting new bridge but the user already has bridges\n");
			exit(-1);			
		}
#endif		

		int numBridgesToGive = rng->next(MAX_KNOWN_BRIDGES) + 1;
		userNumKnownBridgeMap[user] = numBridgesToGive;
		for (int i = 0; i < numBridgesToGive; i++) {
			Bridge* b = getBridge();
			userBridgeMap[user] = userBridges;
			userBridgeMap[user][i] = b;								
			UserStackNode* oldTop = bridgeUsersMap.contains(b) ? nullptr : bridgeUsersMap[b]; 
			bridgeUsersMap[b] = new UserStackNode(oldTop, user);
		}			
		return numBridgesToGive;		
	}

	void publishDailyBridgeStats() {
		for(int i = 0; i < bridgeDB.size(); i++) {
			bridgeDB[i]->progressToNextDay();
		}
	}

	void migrateAllUsersOffBridge(Bridge* b) {
		UserStackNode* top = bridgeUsersMap[b];
		while (top) {
			migrateUserOffBridge(top->user, b);
			top = top->next;			
		}

		discardBridge (b);
		if (bridgeDB.size() <= minBridgeDBSize) {
			expandBridgeDB(minBridgeDBSize);
		}			
	}

	void update() {
#ifdef DEBUG1
		printf("Total bridge count = %ld\n", bridgeDB.size());
#endif
	}
};