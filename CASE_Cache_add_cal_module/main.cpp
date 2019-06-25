#include <iostream>
#include <queue>
#include <thread>
#include <future>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#include "big_lru.h"
#include "small_lru.h"
#include "Safe_Queue.h"

#define THREAD_NUM 8
#define CORE_NUM 4

#define SCALE 10000000
//#define BEGIN 9000000 - 1
//#define END 19000000 - 1

#define WRITE 1
#define WRITE_TIMES 100//36//99//36

//#define LRU_1_FIRST 1
#define LRU_2_FIRST 1

using namespace std;

typedef struct LRU_Thread_Arg {
	int LRU_index;
	long long start;
	long long end;
	//long long scale;
	clock_t LRU_start;
	clock_t	LRU_finish;
}LRU_Thread_Arg;

SmallLRU * smalllru[THREAD_NUM];
BigLRU * biglru[THREAD_NUM];
LRU_Thread_Arg * LRU_args[THREAD_NUM];

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
	case_flowid_t flow_id;
	case_bytecnt_t byte_cnt;
    desc_item() {}
	desc_item(string temp) {
		vector<string> record;
		SplitString(temp, record, " ");
		stringstream ss;
		ss << record[0];
		ss >> this->flow_id;
		ss.clear();
		ss << record[1];
		ss >> this->byte_cnt;
	}
};

QUEUE_DATA<desc_item> * buffer_q_LRU_1[THREAD_NUM];
QUEUE_DATA<desc_item> * buffer_q_LRU_2[THREAD_NUM];

#ifdef LRU_1_FIRST
QUEUE_DATA<desc_item> * LRU_1_notification[THREAD_NUM];

void * LRU_2_LOGIC_LRU_1_FIRST (LRU_Thread_Arg * arg) {
	BigLRU * lru2 = biglru[arg->LRU_index];
	long long i = 0;
	struct desc_item temp_LRU_2;
	int flag_LRU_2 = 0;
	case_flowid_t Flow_ID = 0;
	case_bytecnt_t ByteCnt = 0;
	int found = 0;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i ++;
		flag_LRU_2 = buffer_q_LRU_2[arg->LRU_index]->pop_data(&temp_LRU_2);
		if (flag_LRU_2 == 0) {
			Flow_ID = temp_LRU_2.flow_id;
			ByteCnt = temp_LRU_2.byte_cnt;
			if (ByteCnt > SMALL_BYTE_THRES) {
				if ((found = lru2->find(Flow_ID)) != -1){
					lru2->insertFromOut(Flow_ID, ByteCnt, found);
				}else{
					lru2->insertFromSmallLRU(Flow_ID, ByteCnt);	
				}
			}
			else {
				if ((found = lru2->find(Flow_ID)) != -1) {
					lru2->insertFromOut(Flow_ID, ByteCnt, found);
				}
				else {
					LRU_1_notification[arg->LRU_index]->push_data(temp_LRU_2);
				}
			}
		}
    }
	if (WRITE) {
		lru2->writeAllToSRAM();
		string str1 = "sram_estimate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru2->sram->writeToFile(str1+str3+str4);
	}

#if PRINT_CAL
	lru2->sram->cal_table_write_to_file();
	lru2->sram->write_count_to_file();
	lru2->sram->off_cal_file();
#endif
    return NULL;
}

void * LRU_1_LOGIC_LRU_1_FIRST (LRU_Thread_Arg * arg) {
	SmallLRU * lru1 = smalllru[arg->LRU_index];
	long long i = 0;
	struct desc_item temp_LRU_1;
	int flag_LRU_1 = 0;
	int flag_LRU_1_notification = 0;
	case_flowid_t Flow_ID = 0;
	case_bytecnt_t ByteCnt = 0;
	int found = 0;
	case_bytecnt_t value = 0;
	long long LRU_1_index = 0;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i++;
		flag_LRU_1 = buffer_q_LRU_1[arg->LRU_index]->pop_data(&temp_LRU_1);
		if (flag_LRU_1 == 0) {
			LRU_1_index ++;
			if (LRU_1_index == arg->start) {
				arg->LRU_start = clock();
			}
			if (LRU_1_index == arg->end) {
				arg->LRU_finish = clock();
			}
			Flow_ID = temp_LRU_1.flow_id;
			ByteCnt = temp_LRU_1.byte_cnt;
			found = lru1->find(Flow_ID);
			if (found != -1) {
				value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					temp_LRU_1.byte_cnt = value;
					buffer_q_LRU_2[arg->LRU_index]->push_data(temp_LRU_1);
				}
			}
			else {
				buffer_q_LRU_2[arg->LRU_index]->push_data(temp_LRU_1);
				//lru1->insertNew(Flow_ID, ByteCnt);
			}
		}
		flag_LRU_1_notification = LRU_1_notification[arg->LRU_index]->pop_data(&temp_LRU_1);
		if (flag_LRU_1_notification == 0) {
			Flow_ID = temp_LRU_1.flow_id;
			ByteCnt = temp_LRU_1.byte_cnt;
			if((found = lru1->find(Flow_ID)) != -1){
				value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					temp_LRU_1.byte_cnt = value;
					buffer_q_LRU_2[arg->LRU_index]->push_data(temp_LRU_1);
				}
			}else{
				lru1->insertNew(Flow_ID, ByteCnt);
			}
		}
    }
	if (WRITE) {
		lru1->writeAllToDRAM();
		string str2 = "dram_accurate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru1->dram->writeToFile(str2+str3+str4);
	}

#if PRINT_CAL
	lru1->dram->write_count_to_file();
#endif
	return NULL;
}
#endif

#ifdef LRU_2_FIRST
QUEUE_DATA<desc_item> * LRU_2_notification[THREAD_NUM];

void * LRU_2_LOGIC_LRU_2_FIRST (LRU_Thread_Arg * arg) {
	BigLRU * lru2 = biglru[arg->LRU_index];
	long long i = 0;
	struct desc_item temp_LRU_2;
	int flag_LRU_2 = 0;
	case_flowid_t Flow_ID = 0;
	case_bytecnt_t ByteCnt = 0;
	int found = 0;
	long long LRU_2_index = 0;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i ++;
		flag_LRU_2 = buffer_q_LRU_2[arg->LRU_index]->pop_data(&temp_LRU_2);		
		if (flag_LRU_2 == 0) {
			LRU_2_index ++;
			if (LRU_2_index == arg->start) {
				arg->LRU_start = clock();
			}
			if (LRU_2_index == arg->end) {
				arg->LRU_finish = clock();
			}
			Flow_ID = temp_LRU_2.flow_id;
			ByteCnt = temp_LRU_2.byte_cnt;
			if ((found = lru2->find(Flow_ID)) != -1) {
				lru2->insertFromOut(Flow_ID, ByteCnt, found);
			}
			else {
				buffer_q_LRU_1[arg->LRU_index]->push_data(temp_LRU_2);
			}
		}
		flag_LRU_2 = LRU_2_notification[arg->LRU_index]->pop_data(&temp_LRU_2);
		if (flag_LRU_2 == 0) {
			//insert new
			Flow_ID = temp_LRU_2.flow_id;
			ByteCnt = temp_LRU_2.byte_cnt;
			if ((found = lru2->find(Flow_ID)) != -1) {
				lru2->insertFromOut(Flow_ID, ByteCnt, found);
			}
			else {
				lru2->insertFromSmallLRU(Flow_ID, ByteCnt);	
			}
		}
    }

	if (WRITE) {
		lru2->writeAllToSRAM();
		string str1 = "sram_estimate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru2->sram->writeToFile(str1+str3+str4);
	}

#if PRINT_CAL
	lru2->sram->cal_table_write_to_file();
	lru2->sram->write_count_to_file();
#endif

    return NULL;
}

void * LRU_1_LOGIC_LRU_2_FIRST (LRU_Thread_Arg * arg) {
	SmallLRU * lru1 = smalllru[arg->LRU_index];
	long long i = 0;
	struct desc_item temp_LRU_1;
	int flag_LRU_1 = 0;
	case_flowid_t Flow_ID = 0;
	case_bytecnt_t ByteCnt = 0;
	int found = 0;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i++;
		flag_LRU_1 = buffer_q_LRU_1[arg->LRU_index]->pop_data(&temp_LRU_1);
		if (flag_LRU_1 == 0) {
			Flow_ID = temp_LRU_1.flow_id;
			ByteCnt = temp_LRU_1.byte_cnt;
			if ((found = lru1->find(Flow_ID)) != -1) {
				int value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					temp_LRU_1.byte_cnt = value;
					LRU_2_notification[arg->LRU_index]->push_data(temp_LRU_1);
				}
			}
			else {
				lru1->insertNew(Flow_ID, ByteCnt);
			}
		}  
    }

	if (WRITE) {
		lru1->writeAllToDRAM();
		string str2 = "dram_accurate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru1->dram->writeToFile(str2+str3+str4);
	}
#if PRINT_CAL
	lru1->dram->write_count_to_file();
#endif
	return NULL;
}
#endif

void * LRU_LOGIC (LRU_Thread_Arg * arg) {  
	//并行
#ifdef LRU_1_FIRST
	thread thread_LRU_1_LOGIC(LRU_1_LOGIC_LRU_1_FIRST, arg);
    thread thread_LRU_2_LOGIC(LRU_2_LOGIC_LRU_1_FIRST, arg);
	thread_LRU_1_LOGIC.join();
	thread_LRU_2_LOGIC.join();	
#endif

#ifdef LRU_2_FIRST
	thread thread_LRU_2_LOGIC(LRU_2_LOGIC_LRU_2_FIRST, arg);
	thread thread_LRU_1_LOGIC(LRU_1_LOGIC_LRU_2_FIRST, arg);
    thread_LRU_2_LOGIC.join();
	thread_LRU_1_LOGIC.join();	
#endif

    return NULL;
}

int main() {
	thread LRU_threads[THREAD_NUM];
	long long LRU_scale[THREAD_NUM];
	for (int i = 0;i < THREAD_NUM;i++) {
		LRU_args[i] = new struct LRU_Thread_Arg();
		buffer_q_LRU_1[i] = new QUEUE_DATA<desc_item>(SCALE);
		buffer_q_LRU_2[i] = new QUEUE_DATA<desc_item>(SCALE + 1);
		LRU_args[i]->LRU_index = i;
		LRU_scale[i] = 0;
#ifdef LRU_1_FIRST
		LRU_1_notification[i] = new QUEUE_DATA<desc_item>(SCALE/*/10000*/);
#endif
#ifdef LRU_2_FIRST
		LRU_2_notification[i] = new QUEUE_DATA<desc_item>(SCALE/*/10000*/);
#endif
		smalllru[i] = new SmallLRU();
		biglru[i] = new BigLRU();
		smalllru[i]->init(16 * 1024, 0x3fff, 14);
		biglru[i]->init(112 * 1024, 0xffff, 16);
	}
	//目前使用单线程进行读取
	ifstream readFile("D:\\report_2013.txt");
	string s = "";
	int i = 0;
	while (getline(readFile, s) && i < SCALE) {
		struct desc_item entry(s);
#ifdef LRU_1_FIRST
		buffer_q_LRU_1[i%THREAD_NUM]->push_data(entry);
#endif
#ifdef LRU_2_FIRST
		buffer_q_LRU_2[i%THREAD_NUM]->push_data(entry);
#endif
		LRU_scale[i%THREAD_NUM] ++;
		i++;
	}
	
	printf("ok\n");
	
	for (int i = 0;i < THREAD_NUM;i++) {
		cout << buffer_q_LRU_2[i]->queue_size() << endl;
		//LRU_args[i]->scale = LRU_scale[i];
		LRU_args[i]->start = LRU_scale[i]/4 - 1;
		LRU_args[i]->end = LRU_scale[i]*3/4 - 1;
		LRU_threads[i] =  thread(LRU_LOGIC, LRU_args[i]);
	}

	//thread thread_process_data(process_data);
	//thread_process_data.join();

	clock_t start, finish;
	double totaltime;
	start = clock();  
	for (int i = 0;i < THREAD_NUM;i++) {
		LRU_threads[i].join();
	}
	finish = clock();
	totaltime = (double)(finish - start) / CLOCKS_PER_SEC;

	cout << totaltime << endl;
	double speed = SCALE / totaltime / 1000 / 1000;
	cout << speed << endl;

	/*double thread_time_max = 0;
	for (int i = 0;i < THREAD_NUM;i++) {
		double totaltime_new = (double)(LRU_args[i]->LRU_finish - LRU_args[i]->LRU_start) / CLOCKS_PER_SEC;
		cout << totaltime_new << endl;
		if (thread_time_max < totaltime_new) {
			thread_time_max = totaltime_new;
		}
	}
	cout << "thread_time_max : " << thread_time_max << endl;
	double speed_new = SCALE / 2 / thread_time_max / 1000 /1000;*/

	clock_t thread_start_time_min = LRU_args[0]->LRU_start;
	clock_t thread_end_time_max = LRU_args[0]->LRU_finish;
	for (int i = 1;i < THREAD_NUM;i++) {
		if (thread_start_time_min - LRU_args[i]->LRU_start > 0) {
			thread_start_time_min = LRU_args[i]->LRU_start;
		}
		if (LRU_args[i]->LRU_finish - thread_end_time_max > 0) {
			thread_end_time_max = LRU_args[i]->LRU_finish;
		}
	}
	double totaltime_new = (double)(thread_end_time_max - thread_start_time_min) / CLOCKS_PER_SEC;
	cout << totaltime_new << endl;
	double speed_new = SCALE / 2 / totaltime_new / 1000 /1000;
	
	cout << "speed_new : " << speed_new << endl;

	for (int i = 0;i < THREAD_NUM;i++) {
		delete smalllru[i];
		delete biglru[i];
		delete LRU_args[i];
		delete buffer_q_LRU_1[i];
		delete buffer_q_LRU_2[i];
#ifdef LRU_1_FIRST
		delete LRU_1_notification[i];
#endif
#ifdef LRU_2_FIRST
		delete LRU_2_notification[i];
#endif
	}
    return 0;
}