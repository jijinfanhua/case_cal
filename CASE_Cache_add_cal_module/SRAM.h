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
	llint symb_value;
	int scale;
	S_element() {}
	S_element(llint symb, int sca) :symb_value(symb), scale(sca) {}
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
	long backToRealValue(llint symb, int scale);
#ifdef PRINT_CAL
	void cal_table_write_to_file();
	void write_count_to_file();
	void off_cal_file();
	long write_to_sram_count_new;
	long write_to_sram_count_old;
#endif
};

void SRAM::init() {
	cal = new Calculate();
	cal->init();
#ifdef PRINT_CAL
	write_to_sram_count_new = 0;
	write_to_sram_count_old = 0;
#endif
}

#ifdef PRINT_CAL
void SRAM::cal_table_write_to_file() {
	cal->write_cal_to_file();
}
#endif

void SRAM::insert(int FlowId, int ByteCnt) {
	map<int, S_element>::iterator it;
	it = container.find(FlowId);

	int scale = 0;
	llint symb = 0;

	if (it == container.end()) {
		cal->update(&symb, ByteCnt, &scale);
#ifdef PRINT_CAL
		write_to_sram_count_new += 1;
#endif
		container.insert(make_pair(FlowId, S_element(symb, scale)));
	}
	else {
		S_element temp = it->second;
		scale = temp.scale;
		symb = temp.symb_value;

		container.erase(it);
		
		cal->update(&symb, ByteCnt, &scale);
#ifdef PRINT_CAL
		write_to_sram_count_old += 1;
#endif
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
	FILE * fp = fopen(filename.c_str(), "w");
	map<int, S_element>::iterator it;
	it = container.begin();
	llint esti_value;
	while (it != container.end()) {
		esti_value = cal->backToRealValue(it->second.symb_value, it->second.scale);
		fprintf(fp, "%d\t%lld\n", it->first, esti_value);
		it++;
	}
	fclose(fp);
}

#ifdef PRINT_CAL
void SRAM::write_count_to_file() {
	FILE * fp = fopen("sram_count.txt", "w");
	fprintf(fp, "insert new: %ld\n", write_to_sram_count_new);
	fprintf(fp, "insert old: %ld\n", write_to_sram_count_old);
	fclose(fp);
}
#endif

#ifdef PRINT_CAL
void SRAM::off_cal_file() {
	cal->off_file();
}
#endif

int SRAM::count() {
	return container.size();
}

void SRAM::del_element(int FlowId) {

}

#endif /* SRAM_h */
