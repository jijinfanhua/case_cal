//
//  DRAM.h
//  CASE_Cache
//
//  Created by fengyong on 2019/6/6.
//  Copyright © 2019 fengyong. All rights reserved.
//

#ifndef DRAM_h
#define DRAM_h

#include <queue>
#include <map>
#include "cal.h"

using namespace std;

class DRAM {
private:
	map<case_flowid_t, case_symb_t> container;

public:
	void insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt);
	size_t count();
	void del_element(case_flowid_t FlowId);
	case_symb_t find(case_flowid_t FlowId);
	void writeToFile(string filename);
	void init();
#ifdef PRINT_CAL
	void write_count_to_file();
	long write_to_dram_count_new;
	long write_to_dram_count_old;
#endif
};

void DRAM::init() {
#ifdef PRINT_CAL
	write_to_dram_count_new = 0;
	write_to_dram_count_old = 0;
#endif
}

void DRAM::insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt) {
	//return;
	map<case_flowid_t, case_symb_t>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
#ifdef PRINT_CAL
		write_to_dram_count_new += 1;
#endif
		container.insert(make_pair(FlowId, (case_symb_t)ByteCnt));
	}
	else {
		case_symb_t temp = it->second;
		container.erase(it);
#ifdef PRINT_CAL
		write_to_dram_count_old += 1;
#endif
		container.insert(make_pair(FlowId, (case_symb_t)(ByteCnt + temp)));
	}
}

case_symb_t DRAM::find(case_flowid_t FlowId) {
	map<case_flowid_t, case_symb_t>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
		return -1;
	}
	else {
		return it->second;
	}
}

void DRAM::writeToFile(string filename) {
	FILE * fp = fopen(filename.c_str(), "w");
	map<case_flowid_t, case_symb_t>::iterator it;
	it = container.begin();
	while (it != container.end()) {
		//cout << it->first << " ::" << it->second << endl;
		fprintf(fp, "%u\t%lu\n", it->first, it->second);
		it++;
	}
	fclose(fp);
}

#ifdef PRINT_CAL
void DRAM::write_count_to_file() {
	FILE * fp = fopen("dram_count.txt", "w");
	fprintf(fp, "insert new: %ld\n", write_to_dram_count_new);
	fprintf(fp, "insert old: %ld\n", write_to_dram_count_old);
	fclose(fp);
}
#endif

size_t DRAM::count() {
	return container.size();
}

void DRAM::del_element(case_flowid_t FlowId) {

}

#endif /* DRAM_h */
