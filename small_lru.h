﻿#pragma once
//
//  small_lru.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/5.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef small_lru_h
#define small_lru_h

#include "set_associative.h"
#include "hash_entry.h"
#include <immintrin.h>
#include "DRAM.h"

#define SMALL_BYTE_THRES 8192
#define SMALL_PKT_THRES 9

class SmallLRU {
public:
	CacheSet ** lrutable;
	HashEntry ** hashtable;
	case_flowid_t mask; //for & to get the hash value
	int LRU_SIZE;
	int lru_pointer;
	DRAM * dram;

	CacheSet *head;
	CacheSet *tail;

public:
	void init(int lru_size, case_flowid_t m, int hash_size);
	int find(case_flowid_t FlowId);
	int insertNew(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt);
	pair<case_bytecnt_t, case_pkt_t> insertOld(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt, int found);
	void setLRUTableEntry(int n, int p, int loc);
	void writeAllToDRAM();
};

void SmallLRU::init(int lru_size, case_flowid_t m, int hash_size) {
	LRU_SIZE = lru_size;
	mask = m;
	hashtable = new HashEntry*[(int)(1 << hash_size)];
	lrutable = new CacheSet*[LRU_SIZE];


	dram = new DRAM();
	dram->init();

	head = new CacheSet();
	tail = new CacheSet();

	for (int i = 0; i < LRU_SIZE; i++) {
		lrutable[i] = new CacheSet();
		lrutable[i]->ctr = 0;

#ifdef M128_U32
		for (int j = 0; j < 4; j++) {
			lrutable[i]->flow_id.m128i_u32[j] = 0xffffffff;
			lrutable[i]->usage[j] = 0;
			lrutable[i]->counter[j] = 0;
			//printf("%u\n",lrutable[i]->flow_id.m128i_u32[j]);
		}
#else
		//uint *p = (uint*)(&(lrutable[i]->flow_id));
		for (int j = 0; j < 4; j++) {
			lrutable[i]->flow_id[j] = 0xffffffff;
			lrutable[i]->usage[j] = 0;
			lrutable[i]->counter[j] = 0;
			lrutable[i]->pkt_counter[j] = 0;
			//printf("%u\n",lrutable[i]->flow_id.m128i_u32[j]);
		}
#endif

		lrutable[i]->_previous = -1;
		lrutable[i]->_next = -1;
	}

	int temp = (1 << hash_size);
	for (int i = 0; i < temp; i++) {
		hashtable[i] = new HashEntry();
	}

	lru_pointer = 0;

	head->_previous = -1;
	head->_next = TAIL;
	tail->_previous = HEAD;
	tail->_next = -1;
}

int SmallLRU::find(case_flowid_t FlowId) {
	case_flowid_t hash_id = FlowId & mask;
	int loc = hashtable[hash_id]->cache_loc;
	if (loc == DEFAULT) {
		return -1;
	}

#if SIMD_FUNC
	/*
	 * _mm_set1_epi32: copy one int to four, dst[i+31:i] = a[31:0]
	 * _mm_cmpeq_epi32: compare two 128bits, 31:0, dst[i+31:i] := ( a[i+31:i] == b[i+31:i] ) ? 0xFFFFFFFF : 0
	 * _mm_movemask_epi8: get the lowest bit of each 8bits.
	 * the possible result is:
	 * 		00000000000000000 0000111111111111
	 * 		00000000000000000 1111000011111111
	 * 		00000000000000000 1111111100001111
	 * 		00000000000000000 1111111111110000
	 * 		00000000000000000 1111111111111111
	 * 	_tzcnt_u32:
	 * */
	int found = (_tzcnt_u32((_mm_movemask_epi8(_mm_cmpeq_epi32(lrutable[loc]->flow_id, _mm_set1_epi32(FlowId))) & 0x1111))) >> 2;
#else
	int found = 8;
	for(int i = 0; i < 4; i++){
		if (lrutable[loc]->flow_id[i] == FlowId){
			found = i;
			break;
		}
	}
#endif

	if (found != 8) {
		return found;
	}
	else {
		return -1;
	}
}

/**
* 已经找到，则直接加，移动指针 case_bytecnt_t
*/
pair<case_bytecnt_t, case_pkt_t> SmallLRU::insertOld(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt, int found) {
	case_flowid_t hash_id = Flow_ID & mask;
	int loc = hashtable[hash_id]->cache_loc;
	CacheSet * entry = lrutable[loc];

	entry->counter[found] += ByteCnt;
	entry->pkt_counter[found] += 1;

	if (entry->counter[found] > SMALL_BYTE_THRES || entry->pkt_counter[found] > SMALL_PKT_THRES) {
#ifdef M128_U32
		entry->flow_id.m128i_u32[found] = 0xffffffff;
#else
		//uint *p = (uint*)(&(entry->flow_id));
		entry->flow_id[found] = 0xffffffff;
#endif
		entry->usage[found] = 0;

		case_bytecnt_t temp = entry->counter[found];
		case_pkt_t pkt_temp = entry->pkt_counter[found];

		entry->counter[found] = 0;
		entry->pkt_counter[found] = 0;
		return {temp , pkt_temp};
	}

	entry->usage[found] = ++entry->ctr;

	if (entry->_previous != HEAD) {
		if (entry->_next == TAIL) {
			lrutable[entry->_previous]->_next = TAIL;
			tail->_previous = entry->_previous;
		}
		else if (entry->_next != TAIL) {
			lrutable[entry->_next]->_previous = entry->_previous;
			lrutable[entry->_previous]->_next = entry->_next;
		}

		lrutable[head->_next]->_previous = loc;
		entry->_next = head->_next;
		entry->_previous = HEAD;
		head->_next = loc;
	}
	return {0,0};
}

/*
未找到，添加新的：
1. hashtable的loc还没有值：若lru满，则从最旧的里面插入，将弹出的victim记到DRAM；若lru未满，则直接插入lru即可。
2. hashtable的loc有值，说明之前已经有流，则在这个loc上替换最旧的即可。
*/
int SmallLRU::insertNew(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt) {

	if (ByteCnt >= SMALL_BYTE_THRES) {// 上来就大于阈值，直接返回
		return ByteCnt;
	}

	case_flowid_t hash_id = Flow_ID & mask;

	if (hashtable[hash_id]->cache_loc == DEFAULT) {
		if (lru_pointer >= LRU_SIZE) {
			int loc = tail->_previous;
			CacheSet *entry = lrutable[loc];

			int imin = 0, minv = entry->usage[0];
			for (int i = 0; i < 4; i++) {
				if (entry->usage[i] < minv) {
					imin = i;
					minv = entry->usage[i];
				}
			}

#ifdef M128_U32
			dram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
			entry->flow_id.m128i_u32[imin] = Flow_ID;
			//printf("insertNew1: insert flow[%u] in loc[%d][%d]!\n", Flow_ID, loc, imin);
#else
			//uint *p = (uint*)(&(entry->flow_id));
			if(entry->flow_id[imin]!=0xffffffff)
				dram->insert(entry->flow_id[imin], entry->counter[imin], entry->pkt_counter[imin]);
			entry->flow_id[imin] = Flow_ID;
#endif

			entry->counter[imin] = ByteCnt;
			entry->pkt_counter[imin] = 1;
			entry->usage[imin] = ++entry->ctr;

			lrutable[entry->_previous]->_next = TAIL;
			tail->_previous = entry->_previous;

			entry->_next = head->_next;
			lrutable[head->_next]->_previous = loc;
			entry->_previous = HEAD;
			head->_next = loc;

			/* 此处记录hash对应的cache的位置 */
			hashtable[hash_id]->cache_loc = loc;
			return 0;
		}
		else {
			CacheSet * entry = lrutable[lru_pointer];
			int imin = 0, minv = entry->usage[0];
			for (int i = 0; i < 4; i++) {
				if (entry->usage[i] < minv) {
					imin = i;
					minv = entry->usage[i];
				}
			}

#ifdef M128_U32
			if(entry->flow_id.m128i_u32[imin] != 0xffffffff)
				dram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
			entry->flow_id.m128i_u32[imin] = Flow_ID;
			//printf("insertNew2: insert flow[%u] in loc[%d][%d]!\n", Flow_ID, lru_pointer, imin);
#else
			//uint *p = (uint*)(&(entry->flow_id));
			if(entry->flow_id[imin] != 0xffffffff)
				dram->insert(entry->flow_id[imin], entry->counter[imin], entry->pkt_counter[imin]);
			entry->flow_id[imin] = Flow_ID;
#endif

			entry->counter[imin] = ByteCnt;
			entry->pkt_counter[imin] = 1;
			entry->usage[imin] = ++entry->ctr;

			setLRUTableEntry(head->_next, HEAD, lru_pointer);

			if (head->_next != TAIL) {
				lrutable[head->_next]->_previous = lru_pointer;
				head->_next = lru_pointer;
			}
			else {
				tail->_previous = lru_pointer;
				head->_next = lru_pointer;
			}
			/* 记录hash对应的cache位置 */
			hashtable[hash_id]->cache_loc = lru_pointer;
			lru_pointer++;
			return 0;
		}
	}
	else if (hashtable[hash_id]->cache_loc != DEFAULT) {
		int loc = hashtable[hash_id]->cache_loc;

		CacheSet * entry = lrutable[loc];

		int imin = 0, minv = entry->usage[0];
		for (int i = 0; i < 4; i++) {
			if (entry->usage[i] < minv) {
				imin = i;
				minv = entry->usage[i];
			}
		}

#ifdef M128_U32
		dram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
		entry->flow_id.m128i_u32[imin] = Flow_ID;
		//printf("insertNew3: insert flow[%u] in loc[%d][%d]!\n", Flow_ID, loc, imin);
#else
		//uint *p = (uint*)(&(entry->flow_id));
		dram->insert(entry->flow_id[imin], entry->counter[imin], entry->pkt_counter[imin]);
		entry->flow_id[imin] = Flow_ID;
#endif
		
		entry->counter[imin] = ByteCnt;
		entry->pkt_counter[imin] = 1;
		entry->usage[imin] = ++entry->ctr;

		if (head->_next != loc) {
			if (entry->_next != TAIL) {
				lrutable[entry->_next]->_previous = entry->_previous;
				lrutable[entry->_previous]->_next = entry->_next;
			}
			else {
				tail->_previous = entry->_previous;
				lrutable[entry->_previous]->_next = TAIL;
			}

			lrutable[head->_next]->_previous = loc;
			head->_next = loc;
			setLRUTableEntry(head->_next, HEAD, loc);
		}
		return 0;
	}
	return 0;
}

void SmallLRU::writeAllToDRAM() {
	FILE * fp = fopen("lru1.txt","w");
	case_flowid_t flowid = 0;
	case_bytecnt_t count = 0;
	case_pkt_t pkt_cnt = 0;
	for (int i = 0; i < LRU_SIZE; i++) {
#ifdef M128_U32
		for (int j = 0; j < 4; j++) {
			fprintf(fp,"%u:%lu\t",lrutable[i]->flow_id.m128i_u32[j], lrutable[i]->counter[j]);
			if ((flowid = lrutable[i]->flow_id.m128i_u32[j]) != FLOWID_DEFAULT && (count = lrutable[i]->counter[j]) != COUNTER_DEFAULT) {
				dram->insert(flowid, count);
			}
		}
		fprintf(fp, "\n");
#else
		//uint *p = (uint*)(&(lrutable[i]->flow_id));
		for (int j = 0; j < 4; j++) {
			fprintf(fp,"%u:%lu %u\t", lrutable[i]->flow_id[j], lrutable[i]->counter[j], lrutable[i]->pkt_counter[j]);
			if ((flowid = lrutable[i]->flow_id[j]) != FLOWID_DEFAULT && (count = lrutable[i]->counter[j]) != COUNTER_DEFAULT && ((pkt_cnt = lrutable[i]->pkt_counter[j]) != PKT_COUNTER_DEFAULT)) {
				dram->insert(flowid, count, pkt_cnt);
			}
		}
		fprintf(fp, "\n");
#endif
	}
	fclose(fp);
	return;
}

void SmallLRU::setLRUTableEntry(int n, int p, int loc) {
	lrutable[loc]->_next = n;
	lrutable[loc]->_previous = p;
}

#endif /* small_lru_h */
