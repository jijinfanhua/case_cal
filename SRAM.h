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
#include <unordered_map>
#include "cal.h"

using namespace std;

typedef struct S_element {
public:
	case_symb_t symb_value;
	case_pkt_t pkt_cnt;

	S_element() {}
	S_element(case_symb_t symb, case_pkt_t pkt) :symb_value(symb), pkt_cnt(pkt) {}
}S_element;

class SRAM {
private:
	unordered_map<case_flowid_t, S_element> container;

public:
	void insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt, case_pkt_t pkt);
	size_t count();
	void del_element(case_flowid_t FlowId);
	case_symb_t find(case_flowid_t FlowId);
	void init();
	void writeToFile(string filename);
	//case_symb_t backToRealValue(case_symb_t symb, int scale);
#ifdef PRINT_CAL
	void write_count_to_file();
	long write_to_sram_count_new;
	long write_to_sram_count_old;
#endif
};

void SRAM::init() {
	container.reserve(4096);

#ifdef PRINT_CAL
	write_to_sram_count_new = 0;
	write_to_sram_count_old = 0;
#endif
}

void SRAM::insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt, case_pkt_t PktCnt) {
	unordered_map<case_flowid_t, S_element>::iterator it;
	it = container.find(FlowId);

	if (it == container.end()) {
#ifdef PRINT_CAL
		write_to_sram_count_new += 1;
#endif
		container.insert(make_pair(FlowId, S_element(ByteCnt, PktCnt)));
	}
	else {
		case_symb_t symb = 0;
		case_pkt_t pkt_cnt = 0;
		S_element temp = it->second;

		symb = temp.symb_value;
		pkt_cnt = temp.pkt_cnt;

		container.erase(it);
		
#ifdef PRINT_CAL
		write_to_sram_count_old += 1;
#endif
		container.insert(make_pair(FlowId, S_element(symb + ByteCnt, pkt_cnt + PktCnt)));
	}
}

case_symb_t SRAM::find(case_flowid_t FlowId) {
	unordered_map<case_flowid_t, S_element>::iterator it;
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
	unordered_map<case_flowid_t, S_element>::iterator it;
	it = container.begin();
	double esti_value;
	case_symb_t temp;
	while (it != container.end()) {
		fprintf(fp, "%d\t%lu\t%u\n", it->first, it->second.symb_value, it->second.pkt_cnt);
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

size_t SRAM::count() {
	return container.size();
}

void SRAM::del_element(case_flowid_t FlowId) {

}

#endif /* SRAM_h */
