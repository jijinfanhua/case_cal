#pragma once
//
//  big_lru.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/5.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef big_lru_h
#define big_lru_h

#include "set_associative.h"
#include "hash_entry.h"
#include "SRAM.h"

#define BIG_BYTE_THRES 65535

class BigLRU {
public:
	CacheSet ** lrutable;
	HashEntry ** hashtable;
	case_flowid_t mask; //for & to get the hash value
	int LRU_SIZE;
	int lru_pointer;

	SRAM * sram;

	CacheSet *head;
	CacheSet *tail;

public:
	void init(int lru_size, case_flowid_t m, int hash_size);
	int find(case_flowid_t Flow_ID);
	int insertFromOut(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt, int found);
	void setLRUTableEntry(int n, int p, int loc);
	int add(case_flowid_t Flow_id, int location, case_bytecnt_t ByteCnt, int found);
	int insertFromSmallLRU(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt);
	void writeAllToSRAM();
};

void BigLRU::init(int lru_size, case_flowid_t m, int hash_size) {
	LRU_SIZE = lru_size;
	mask = m;
	lrutable = new CacheSet*[LRU_SIZE];
	hashtable = new HashEntry*[(int)(1 << hash_size)];
	head = new CacheSet();
	tail = new CacheSet();
	sram = new SRAM();
	sram->init();
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
		uint *p = (uint*)(&(lrutable[i]->flow_id));
		for (int j = 0; j < 4; j++) {
			p[j] = 0xffffffff;
			lrutable[i]->usage[j] = 0;
			lrutable[i]->counter[j] = 0;
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


int BigLRU::find(case_flowid_t Flow_ID) {
	case_flowid_t hash_id = Flow_ID & mask;
	int loc = hashtable[hash_id]->cache_loc;
	if (loc == DEFAULT) {
		return -1;
	}
	//printf("before found\n");

#if SIMD_FUNC
	int found = _tzcnt_u32((_mm_movemask_epi8(_mm_cmpeq_epi32(lrutable[loc]->flow_id, _mm_set1_epi32(Flow_ID))) & 0x1111)) >> 2;
#else
	int found = 8;
	for(int i = 0; i < 4; i++){
		if (((uint*)&(lrutable[loc]->flow_id))[i] == Flow_ID){
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

int BigLRU::insertFromOut(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt, int found) {
	case_flowid_t hash_id = Flow_ID & mask;
	add(Flow_ID, hashtable[hash_id]->cache_loc, ByteCnt, found);
	return 1;
}


/**
超过LRU2阈值与不超过LRU2阈值
*/
int BigLRU::add(case_flowid_t Flow_id, int location, case_bytecnt_t ByteCnt, int found) {
	CacheSet * entry = lrutable[location];
	if (entry->counter[found] + ByteCnt <= BIG_BYTE_THRES) {
		entry->counter[found] += ByteCnt;

		entry->usage[found] = ++entry->ctr;// 四路组相连中最新的一个

		if (entry->_next == TAIL && entry->_previous != HEAD) {
			lrutable[entry->_previous]->_next = TAIL;
			tail->_previous = entry->_previous;
		}
		else if (entry->_next != TAIL && entry->_previous != HEAD) {
			lrutable[entry->_next]->_previous = entry->_previous;
			lrutable[entry->_previous]->_next = entry->_next;
		}
		entry->_next = head->_next;
		entry->_previous = HEAD;
		lrutable[head->_next]->_previous = location;
		head->_next = location;
		return 0;
	}
	else {
		// write to SRAM and save the flow id
		sram->insert(Flow_id, entry->counter[found] + ByteCnt);

		if (entry->_previous != HEAD) {
			if (entry->_next != TAIL) {
				lrutable[entry->_next]->_previous = entry->_previous;
				lrutable[entry->_previous]->_next = entry->_next;
			}
			else if (entry->_next == TAIL) {
				lrutable[entry->_previous]->_next = TAIL;
				tail->_previous = entry->_previous;
			}

			entry->_next = head->_next;
			entry->_previous = HEAD;
			lrutable[head->_next]->_previous = location;
			head->_next = location;
		}

		entry->counter[found] = 0;
		entry->usage[found] = ++entry->ctr;

		return 0;
	}
	return 0;
}

int BigLRU::insertFromSmallLRU(case_flowid_t Flow_ID, case_bytecnt_t ByteCnt) {
	//LRU2中肯定没有这一项
	//1. 如果LRU2没有满，则直接放在lru_index，调整相应的数值；
	//2. 如果LRU2已经满了，也是放

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

			//TODO: the original should be evicted to SRAM
			//entry->flow_id.m128i_u32[imin]    entry->counter[imin]
#ifdef M128_U32
			sram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
			entry->flow_id.m128i_u32[imin] = Flow_ID;
#else
			uint *p = (uint*)(&(entry->flow_id));
			sram->insert(p[imin], entry->counter[imin]);
			p[imin] = Flow_ID;
#endif
			//printf("%d\t%d\n", entry->flow_id.m128i_u32[imin], entry->counter[imin]);

			
			entry->counter[imin] = ByteCnt;
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
			if(entry->flow_id.m128i_u32[imin]!=0xffffffff){
				sram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
			}

			entry->flow_id.m128i_u32[imin] = Flow_ID;
#else
			uint *p = (uint*)(&(entry->flow_id));
			if(p[imin] != FLOWID_DEFAULT)
				sram->insert(p[imin], entry->counter[imin]);

			p[imin] = Flow_ID;
#endif
			
			entry->counter[imin] = ByteCnt;
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
		if(entry->flow_id.m128i_u32[imin] != 0xffffffff){
			sram->insert(entry->flow_id.m128i_u32[imin], entry->counter[imin]);
		}
		
		entry->flow_id.m128i_u32[imin] = Flow_ID;
#else
		uint *p = (uint*)(&(entry->flow_id));
		sram->insert(p[imin], entry->counter[imin]);

		p[imin] = Flow_ID;
#endif
		entry->counter[imin] = ByteCnt;
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

void BigLRU::writeAllToSRAM() {
	FILE * fp = fopen("lru2.txt","w");
	case_flowid_t flowid = 0;
	case_bytecnt_t count = 0;
	for (int i = 0; i < LRU_SIZE; i++) {
#ifdef M128_U32
		for (int j = 0; j < 4; j++) {
			fprintf(fp,"%u:%lu\t",lrutable[i]->flow_id.m128i_u32[j], lrutable[i]->counter[j]);
			if ((flowid = lrutable[i]->flow_id.m128i_u32[j]) != FLOWID_DEFAULT && (count = lrutable[i]->counter[j]) != COUNTER_DEFAULT) {
				sram->insert(flowid, count);
			}
		}
		fprintf(fp, "\n");
#else
		uint *p = (uint*)(&(lrutable[i]->flow_id));
		for (int j = 0; j < 4; j++) {
			fprintf(fp,"%u:%lu\t",p[j], lrutable[i]->counter[j]);
			if ((flowid = p[j]) != FLOWID_DEFAULT && (count = lrutable[i]->counter[j]) != COUNTER_DEFAULT) {
				sram->insert(flowid, count);
			}
		}
		fprintf(fp, "\n");
#endif
	}
	fclose(fp);
	return;
}

void BigLRU::setLRUTableEntry(int n, int p, int loc) {
	lrutable[loc]->_next = n;
	lrutable[loc]->_previous = p;
}

#endif /* big_lru_h */
