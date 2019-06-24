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

using namespace std;

class DRAM {
private:
	map<int, int> container;

public:
	void insert(int FlowId, int ByteCnt);
	int count();
	void del_element(int FlowId);
	int find(int FlowId);
	void writeToFile(string filename);
	void init();
	void write_count_to_file();
	long write_to_dram_count_new;
	long write_to_dram_count_old;
};

void DRAM::init() {
	write_to_dram_count_new = 0;
	write_to_dram_count_old = 0;
}

void DRAM::insert(int FlowId, int ByteCnt) {
	//return;
	map<int, int>::iterator it;
	it = container.find(FlowId);
	if (it == container.end()) {
		write_to_dram_count_new += 1;
		container.insert(make_pair(FlowId, ByteCnt));
	}
	else {
		int temp = it->second;
		container.erase(it);
		write_to_dram_count_old += 1;
		container.insert(make_pair(FlowId, ByteCnt + temp));
	}
}

int DRAM::find(int FlowId) {
	map<int, int>::iterator it;
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
	map<int, int>::iterator it;
	it = container.begin();
	while (it != container.end()) {
		//cout << it->first << " ::" << it->second << endl;
		fprintf(fp, "%d\t%d\n", it->first, it->second);
		it++;
	}
	fclose(fp);
}

void DRAM::write_count_to_file() {
	FILE * fp = fopen("dram_count.txt", "w");
	fprintf(fp, "insert new: %ld\n", write_to_dram_count_new);
	fprintf(fp, "insert old: %ld\n", write_to_dram_count_old);
	fclose(fp);
}

int DRAM::count() {
	return container.size();
}

void DRAM::del_element(int FlowId) {

}

#endif /* DRAM_h */
