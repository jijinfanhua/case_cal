#pragma once
//
//  hash_entry.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/5.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef hash_entry_h
#define hash_entry_h

#define DEFAULT -1

typedef enum LRULOC { S_LRU, B_LRU, NONE };

class HashEntry {
public:
	//int flow_id; // the original id of flow
	//LRULOC lru_loc;// small flow lru or big flow lru
	int cache_loc;// the index in lru

public:
	//HashEntry(int f_id, LRULOC l_loc, int c_loc) : flow_id(f_id), lru_loc(l_loc), cache_loc(c_loc) {};
	//HashEntry() : flow_id(DEFAULT), lru_loc(NONE), cache_loc(DEFAULT) {};
	HashEntry(int c_loc) : cache_loc(c_loc) {};
	HashEntry() : cache_loc(DEFAULT) {};
};

#endif /* hash_entry_h */