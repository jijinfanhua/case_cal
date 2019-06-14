//
//  parse_and_read.cpp
//  CASE_Cache
//
//  Created by fengyong on 2019/6/9.
//  Copyright © 2019 fengyong. All rights reserved.
//

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
//#include <pthread.h>
#include <thread>
#include <limits>
#include <Windows.h>

#include "big_lru.h"
#include "small_lru.h"

//#include <arpa/inet.h>


#define THREAD_NUM 8

#define CORE_NUM 4

#define PACKET_MAX 10000000

#define SCALE 100//10000000//20000

using namespace std;

long long total_len = 0;

SmallLRU * smalllru[THREAD_NUM];
BigLRU * biglru[THREAD_NUM];

void SplitString(const string& s, vector<string>& v, const string& c) {
	string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (string::npos != pos2) {
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

struct desc_item {
	unsigned long item_index;

	//in_addr ip_src;
	//in_addr ip_des;
	//string ip_src;
	///string ip_des;
	//unsigned short port_src;
	//unsigned short port_des;
	//unsigned char layer_4_proto; //1 UDP

	unsigned short total_length;

	desc_item() {}
	//desc_item(unsigned long i_i, string s_ip, string d_ip, unsigned short s_p, unsigned short d_p, unsigned char p, unsigned short t_l) {
	//	this->item_index = i_i;

	//	this->ip_src = s_ip;
	//	this->ip_des = d_ip;
	//	this->port_src = s_p;
	//	this->port_des = d_p;
	//	this->layer_4_proto = p;

	//	this->total_length = t_l;
	//}
	desc_item(string temp) {
		vector<string> record;
		SplitString(temp, record, " ");
		stringstream ss;
		ss << record[0];
		ss >> this->item_index;
		//cout << this->item_index << " ";

		ss.clear();
		/*if (record[1] == "17") {
			this->layer_4_proto = '1'; //UDP
		}

		this->ip_src = record[2];
		this->ip_des = record[3];
		//cout << this->ip_src << " " << this->ip_des << " ";

		ss.clear();
		ss << record[4];
		ss >> this->port_src;
		//cout << this->port_src << " ";

		ss.clear();
		ss << record[5];
		ss >> this->port_des;
		//cout << this->port_des << " ";

		ss.clear();*/
		ss << record[1];
		ss >> this->total_length;
		//cout << this->total_length << endl;
	}
};

struct desc_item buffer[SCALE];

typedef struct ThreadArg {
	long long base;
	long long length;
	long long sum;
}ThreadArg;

void * case_func(ThreadArg * arg) {
	//SetThreadAffinityMask(GetCurrentThread(),arg->thread_id);
	long long begin = arg->base;
	long long end = arg->base + arg->length;

	long long index = arg->base / (SCALE / THREAD_NUM);
	printf("index: %d\n", index);
	SmallLRU * lru1 = smalllru[index];
	BigLRU * lru2 = biglru[index];

	//SmallLRU * lru1 = new SmallLRU();
	//lru1->init(16 * 1024, 0x3fff, 14);
	//BigLRU * lru2 = new BigLRU();
	//lru2->init(112 * 1024, 0x3fff, 14);

	int Flow_ID = 0, ByteCnt = 0;
	for (long long i = begin; i < end; i++) {
		Flow_ID = buffer[i].item_index;
		ByteCnt = buffer[i].total_length;
		int found = 0;
		if ((found = lru2->find(Flow_ID)) != -1) {
			lru2->insertFromOut(Flow_ID, ByteCnt, found);
		}
		else {
			found = lru1->find(Flow_ID);
			if (found != -1) {
				int value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					lru2->insertFromSmallLRU(Flow_ID, value);
				}
			}
			else {
				lru1->insertNew(Flow_ID, ByteCnt);
			}
		}
	}
	string str1 = "sram_estimate_value_";
	string str2 = "dram_accurate_value_";
	string str3 = std::to_string(index);
	string str4 = ".txt";
	lru1->dram->writeToFile(str2+str3+str4);
	lru2->sram->writeToFile(str1+str3+str4);

	return NULL;
}

struct Packet {
public:
	Packet(int id, int cnt) : Flow_id(id), ByteCnt(id) {};
	Packet() :Flow_id(0), ByteCnt(0) {};
	int Flow_id;
	int ByteCnt;
};

int main() {

	//struct desc_item buffer[SCALE];

	//load file
	ifstream readFile("D:\\report_2013.txt");
	string s = "";
	int i = 0;
	while (getline(readFile, s) && i < SCALE) {
		struct desc_item entry(s);
		buffer[i] = entry;
		i++;
	}
	printf("ok\n");

	thread threads[THREAD_NUM];
	ThreadArg args[THREAD_NUM];
	DWORD_PTR thread_mask[8];
	thread_mask[0] = 0x01;
	for (int i = 1;i < 8;i++) {
		thread_mask[i] = thread_mask[i] * 2; 
	}
	for (int i = 0; i < THREAD_NUM; i++) {
		smalllru[i] = new SmallLRU();
		biglru[i] = new BigLRU();
		smalllru[i]->init(16 * 1024, 0x3fff, 14);
		biglru[i]->init(112 * 1024, 0xffff, 16);
		int offset = (SCALE / THREAD_NUM) * i;
		args[i].base = offset;
		args[i].length = min(SCALE - offset, SCALE / THREAD_NUM);
		threads[i] = thread(case_func, &args[i]);
		SetThreadAffinityMask(threads[i].native_handle(),thread_mask[i%CORE_NUM]);
	}

	clock_t start, finish;
	double totaltime;
	start = clock();
	/*for (int i = 0;i < SCALE;i++) {
	test(buffer[i]);
	}*/

	for (int i = 0; i < THREAD_NUM; ++i) {
		threads[i].join();
	}
	finish = clock();
	totaltime = (double)(finish - start) / CLOCKS_PER_SEC;

	cout << totaltime << endl;
	double speed = PACKET_MAX / totaltime / 1000 / 1000;
	cout << speed << endl;

	//cout << total_len / 10 << endl;
	//long long parallelMethodSum = 0;
	//for (int i = 0; i < THREAD_NUM; ++i) {
	//	parallelMethodSum += args[i].sum;
	//}
	//cout << parallelMethodSum / 10 << endl;

	return 0;
}
