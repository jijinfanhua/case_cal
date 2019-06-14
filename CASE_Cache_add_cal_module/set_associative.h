#pragma once
//
//  set_associative.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/5.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef set_associative_h
#define set_associative_h

#include <immintrin.h>

#define DEFAULT_POINTER -2
#define DEFAULT_LOC -1

#define HEAD -2
#define TAIL -3

class CacheSet {
public:
	//CacheSet(int previous, int next):_previous(previous), _next(next){};
	__m128i flow_id;
	unsigned int counter[4];
	int usage[4];
	int ctr;
	int _previous;
	int _next;
};


#endif /* set_associative_h */
