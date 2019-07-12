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
#include <unordered_map>
#include "cal.h"

using namespace std;

struct less_case_flowid_t {
    bool operator()(const case_flowid_t & x, const case_flowid_t & y) const
    {
        return ((int)x < (int)y);
    }
};

typedef struct D_element {
public:
	case_symb_t symb_value;
	case_pkt_t pkt_cnt;

	D_element() {}
	D_element(case_symb_t symb, case_pkt_t pkt) :symb_value(symb), pkt_cnt(pkt) {}
}D_element;

class DRAM {
private:
	unordered_map<case_flowid_t, D_element> container;

public:
	void insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt, case_pkt_t PktCnt);
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

	container.reserve(4096);

#ifdef PRINT_CAL
	write_to_dram_count_new = 0;
	write_to_dram_count_old = 0;
#endif
}

void DRAM::insert(case_flowid_t FlowId, case_bytecnt_t ByteCnt, case_pkt_t PktCnt) {
	//return;
	unordered_map<case_flowid_t, D_element>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
#ifdef PRINT_CAL
		write_to_dram_count_new += 1;
#endif
		container.insert(make_pair(FlowId, D_element((case_symb_t)ByteCnt, PktCnt)));
	}
	else {
		case_symb_t temp = it->second.symb_value;
		case_pkt_t pkt_temp = it->second.pkt_cnt;
		container.erase(it);
#ifdef PRINT_CAL
		write_to_dram_count_old += 1;
#endif
		container.insert(make_pair(FlowId, D_element((case_symb_t)(ByteCnt + temp), PktCnt + pkt_temp)));
	}
}

case_symb_t DRAM::find(case_flowid_t FlowId) {
	unordered_map<case_flowid_t, D_element>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
		return -1;
	}
	else {
		return it->second.symb_value;
	}
}

void DRAM::writeToFile(string filename) {
	FILE * fp = fopen(filename.c_str(), "w");
	unordered_map<case_flowid_t, D_element>::iterator it;
	it = container.begin();
	while (it != container.end()) {
		//cout << it->first << " ::" << it->second << endl;
		fprintf(fp, "%u\t%lu\t%u\n", it->first, it->second.symb_value, it->second.pkt_cnt);
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
