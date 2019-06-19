#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <future>
#include <string>
#include <sstream>
#include <fstream>
#include <Windows.h>

#include "big_lru.h"
#include "small_lru.h"
#include "Safe_Queue.h"

#define THREAD_NUM 8
#define CORE_NUM 4

#define SCALE 10000000

#define WRITE 0
#define WRITE_TIMES 1

using namespace std;

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
    unsigned long flow_id;
    unsigned short byte_cnt;
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
QUEUE_DATA<desc_item> * LRU_2_notifications[THREAD_NUM];

typedef struct LRU_Thread_Arg {
	int LRU_index;
}LRU_Thread_Arg;

/*void * process_data () {
	int j = 0;
    while (j < SCALE) {
        buffer_q_LRU_2.push_data(buffer[j]);
        j++;
    }
    return NULL;
}*/

/*
int LRU_1_max_length = 0;
int LRU_2_notifications_max_length = 0;
*/

void * LRU_2_LOGIC (LRU_Thread_Arg * arg) {
	BigLRU * lru2 = biglru[arg->LRU_index];
	int i = 0;
	//如果要写文件要保证运行足够长的时间
	//cout << SCALE/THREAD_NUM*WRITE_TIMES << endl;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i ++;
		struct desc_item temp_LRU_2;
		int flag_LRU_2 = buffer_q_LRU_2[arg->LRU_index]->pop_data(&temp_LRU_2);
		struct desc_item temp_LRU_2_notifications;
		int flag_LRU_2_notifications = LRU_2_notifications[arg->LRU_index]->pop_data(&temp_LRU_2_notifications);
		if (flag_LRU_2 == 0) {
			int Flow_ID = temp_LRU_2.flow_id;
			int ByteCnt = temp_LRU_2.byte_cnt;
			int found = 0;
			if ((found = lru2->find(Flow_ID)) != -1) {
				lru2->insertFromOut(Flow_ID, ByteCnt, found);
			}
			else {
				buffer_q_LRU_1[arg->LRU_index]->push_data(temp_LRU_2);
				/*int length = buffer_q_LRU_1[arg->LRU_index]->queue_size();
				if (length > LRU_1_max_length)
					LRU_1_max_length = length;*/
			}
		}
		if (flag_LRU_2_notifications == 0) {
			//insert new
			int Flow_ID = temp_LRU_2_notifications.flow_id;
			int ByteCnt = temp_LRU_2_notifications.byte_cnt;
			lru2->insertFromSmallLRU(Flow_ID, ByteCnt);
		}
    }
	//cout << "buffer_q_LRU_1 : " << LRU_1_max_length << endl;
	if (WRITE) {
		lru2->writeAllToSRAM();
		string str1 = "sram_estimate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru2->sram->writeToFile(str1+str3+str4);
	}
    return NULL;
}

void * LRU_1_LOGIC (LRU_Thread_Arg * arg) {
	SmallLRU * lru1 = smalllru[arg->LRU_index];
	int i = 0;
	while (true && i < SCALE/THREAD_NUM*WRITE_TIMES) {
		i++;
		struct desc_item temp_LRU_1;
		int flag_LRU_1 = buffer_q_LRU_1[arg->LRU_index]->pop_data(&temp_LRU_1);
		if (flag_LRU_1 == 0) {
			int Flow_ID = temp_LRU_1.flow_id;
			int ByteCnt = temp_LRU_1.byte_cnt;
			int found = lru1->find(Flow_ID);
			if (found != -1) {
				int value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					temp_LRU_1.byte_cnt = value;
					LRU_2_notifications[arg->LRU_index]->push_data(temp_LRU_1);
					/*int length = LRU_2_notifications[arg->LRU_index]->queue_size();
					if (length > LRU_2_notifications_max_length)
						LRU_2_notifications_max_length = length;*/
				}
			}
			else {
				lru1->insertNew(Flow_ID, ByteCnt);
			}
		}  
    }
	//cout << "LRU_2_notifications_max_length : " << LRU_2_notifications_max_length << endl;
	if (WRITE) {
		lru1->writeAllToDRAM();
		string str2 = "dram_accurate_value_";
		string str3 = std::to_string(arg->LRU_index);
		string str4 = ".txt";
		lru1->dram->writeToFile(str2+str3+str4);
	}
	return NULL;
}

void * LRU_LOGIC (LRU_Thread_Arg * arg) {  
	//并行
    thread thread_LRU_2_LOGIC(LRU_2_LOGIC, arg);
	thread thread_LRU_1_LOGIC(LRU_1_LOGIC, arg);
	thread_LRU_2_LOGIC.join();
	thread_LRU_1_LOGIC.join();
    return NULL;
}

int main() {
	thread LRU_threads[THREAD_NUM];
	LRU_Thread_Arg LRU_args[THREAD_NUM];
	DWORD_PTR thread_mask[8];
	thread_mask[0] = 0x01;
	for (int i = 1;i < 8;i++) {
		thread_mask[i] = thread_mask[i] * 2; 
	}
	for (int i = 0;i < THREAD_NUM;i++) {
		LRU_args[i].LRU_index = i;
		buffer_q_LRU_1[i] = new QUEUE_DATA<desc_item>(SCALE/2);
		buffer_q_LRU_2[i] = new QUEUE_DATA<desc_item>(SCALE);
		LRU_2_notifications[i] = new QUEUE_DATA<desc_item>(SCALE/10000);
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
		//cout << i%THREAD_NUM << endl;
		buffer_q_LRU_2[i%THREAD_NUM]->push_data(entry);
		i++;
	}
	printf("ok\n");
	
	for (int i = 0;i < THREAD_NUM;i++) {
		//cout << i << " : " << buffer_q_LRU_2[i]->queue_size() << endl;
		LRU_threads[i] =  thread(LRU_LOGIC, &LRU_args[i]);
		//SetThreadAffinityMask(LRU_threads[i].native_handle(),thread_mask[i%CORE_NUM]);
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

	for (int i = 0;i < THREAD_NUM;i++) {
		delete smalllru[i];
		delete biglru[i];
		delete buffer_q_LRU_1[i];
		delete buffer_q_LRU_2[i];
		delete LRU_2_notifications[i];
	}
    return 0;
}