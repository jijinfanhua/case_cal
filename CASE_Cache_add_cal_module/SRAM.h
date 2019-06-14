//
//  SRAM.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/6.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef SRAM_h
#define SRAM_h

#include <queue>
#include <map>
#include "cal.h"

using namespace std;

typedef struct S_element {
public:
	long symb_value;
	int scale;
	S_element() {}
	S_element(long symb, int sca) :symb_value(symb), scale(sca) {}
};

class SRAM {
private:
	map<int, S_element> container;
	Calculate *cal;

public:
	void insert(int FlowId, int ByteCnt);
	int count();
	void del_element(int FlowId);
	long find(int FlowId);
	void init();
	void writeToFile(string filename);
	long backToRealValue(int symb, int scale);
};

void SRAM::init() {
	cal = new Calculate();
	cal->init();
}

void SRAM::insert(int FlowId, int ByteCnt) {
	map<int, S_element>::iterator it;
	it = container.find(FlowId);

	int scale = 0;
	long symb = 0;

	if (it == container.end()) {
		cal->update(&symb, ByteCnt, &scale);
		container.insert(make_pair(FlowId, S_element(symb, scale)));
	}
	else {
		S_element temp = it->second;
		scale = temp.scale;
		symb = temp.symb_value;

		container.erase(it);
		
		cal->update(&symb, ByteCnt, &scale);
		container.insert(make_pair(FlowId, S_element(symb, scale)));
	}
}

long SRAM::find(int FlowId) {
	map<int, S_element>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
		return -1;
	}
	else {
		return it->second.symb_value;
	}
}

void SRAM::writeToFile(string filename) {
	FILE * fp = fopen(filename.c_str(), "a+");
	map<int, S_element>::iterator it;
	it = container.begin();
	int esti_value;
	while (it != container.end()) {
		esti_value = cal->backToRealValue(it->second.symb_value, it->second.scale);
		fprintf(fp, "%d\t%ld\n", it->first, esti_value);
		it++;
	}
	fclose(fp);
}

int SRAM::count() {
	return container.size();
}

void SRAM::del_element(int FlowId) {

}

#endif /* SRAM_h */
