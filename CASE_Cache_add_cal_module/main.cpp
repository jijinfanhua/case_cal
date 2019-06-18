#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <future>
#include <string>
#include <sstream>
#include <fstream>

#include "thread_safe_queue.h"
#include "big_lru.h"
#include "small_lru.h"
#include "Queue.h"

#define SCALE 10000000
#define THREAD_NUM 1
#define WRITE 1

using namespace std;

SmallLRU * smalllru;
BigLRU * biglru;

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

ThreadSafeQueue<struct desc_item, std::condition_variable, std::mutex> buffer_q_LRU_1;
//ThreadSafeQueue<struct desc_item, std::condition_variable, std::mutex> buffer_q_LRU_2;

//Queue<desc_item> buffer_q_LRU_1(SCALE);
//mutex LRU_1_lock;

Queue<desc_item> buffer_q_LRU_2(SCALE);

//从LRU_1来的新大流数据
ThreadSafeQueue<struct desc_item, std::condition_variable, std::mutex> LRU_2_notifications;
//Queue<desc_item> LRU_2_notifications(SCALE);
//mutex LRU_2_notifications_lock;


struct desc_item buffer[SCALE];
long long buffer_index = 0;

void * process_data () {
	//可在此处（读取完成）加入起始时间戳，从而避免测量时间算入读取时间
	int j = 0;
    while (j < SCALE) {
        //buffer_q_LRU_1.Push(buffer[j]);
        buffer_q_LRU_2.push(buffer[j]);
        j++;
    }
    return NULL;
}

#if(0)
void * sub_1_LRU_2_LOGIC(BigLRU * lru2) {
	//LRU_2逻辑Part1
	//读取原始数据不用通知LRU1
	while (/*true*/!buffer_q_LRU_2.IsEmpty()/*buffer_index < SCALE*/) {
        struct desc_item temp = *(buffer_q_LRU_2.WaitAndPop());
		//struct desc_item temp = buffer[buffer_index];
		int Flow_ID = temp.flow_id;
		int ByteCnt = temp.byte_cnt;
		int found = 0;
		if ((found = lru2->find(Flow_ID)) != -1) {
			lru2->insertFromOut(Flow_ID, ByteCnt, found);
		}
		else {
			//LRU_1_notifications.Push(temp);
			buffer_q_LRU_1.Push(temp);
		}	
    }
	//cout << "LRU_2 EMPTY: " <<  buffer_q_LRU_2.IsEmpty() << endl;
	return NULL;
}

void * sub_2_LRU_2_LOGIC(BigLRU * lru2) {
	//LRU_2逻辑Part2
	while (/*true*/!LRU_2_notifications.IsEmpty()) {
		struct desc_item temp = *(LRU_2_notifications.WaitAndPop());
		//insert new
		int Flow_ID = temp.flow_id;
		int ByteCnt = temp.byte_cnt;
		lru2->insertFromSmallLRU(Flow_ID, ByteCnt);
	}
	//cout << "LRU_2_notifications EMPTY: " <<  LRU_2_notifications.IsEmpty() << endl;
    return NULL;
}
#endif

void * sub_3_LRU_2_LOGIC(BigLRU * lru2) {
	while (true) {
		bool LRU_1_empty = buffer_q_LRU_1.IsEmpty();
		bool LRU_2_empty = buffer_q_LRU_2.IsEmpty();
		bool LRU_2_notifications_empty = LRU_2_notifications.IsEmpty();
		if (!LRU_2_empty) {
			//struct desc_item temp_LRU_1 = *(buffer_q_LRU_2.WaitAndPop());
			struct desc_item temp = buffer_q_LRU_2.pop();
			int Flow_ID = temp.flow_id;
			int ByteCnt = temp.byte_cnt;
			int found = 0;
			if ((found = lru2->find(Flow_ID)) != -1) {
				lru2->insertFromOut(Flow_ID, ByteCnt, found);
			}
			else {
				buffer_q_LRU_1.Push(temp);
			}
		}
		else if (!LRU_2_notifications_empty) {
			struct desc_item temp_LRU_2_notifications = *(LRU_2_notifications.WaitAndPop());
			//insert new
			int Flow_ID = temp_LRU_2_notifications.flow_id;
			int ByteCnt = temp_LRU_2_notifications.byte_cnt;
			lru2->insertFromSmallLRU(Flow_ID, ByteCnt);
		}
		else if (LRU_1_empty && LRU_2_empty && LRU_2_notifications_empty){
			if (WRITE) {
				//cout << "LRU_2 : " << buffer_q_LRU_1.IsEmpty() << " : " << buffer_q_LRU_2.IsEmpty() << " : " << LRU_2_notifications.IsEmpty() << endl;
				lru2->writeAllToSRAM();
				string str1 = "sram_estimate_value_";
				string str3 = "0";
				string str4 = ".txt";
				lru2->sram->writeToFile(str1+str3+str4);
			}
			break;
		}
    }
	//cout << "LRU_2 EMPTY: " <<  buffer_q_LRU_2.IsEmpty() << endl;
	return NULL;
}

void * LRU_2_LOGIC () {
	BigLRU * lru2 = biglru;
    /*thread sub_1_thread_LRU_2_LOGIC(sub_1_LRU_2_LOGIC, lru2);
    thread sub_2_thread_LRU_2_LOGIC(sub_2_LRU_2_LOGIC, lru2);
	sub_1_thread_LRU_2_LOGIC.join();
    sub_2_thread_LRU_2_LOGIC.join();*/
	thread sub_3_thread_LRU_2_LOGIC(sub_3_LRU_2_LOGIC, lru2);
	sub_3_thread_LRU_2_LOGIC.join();
    return NULL;
}

void * LRU_1_LOGIC () {
	SmallLRU * lru1 = smalllru;
	while (true) {
		bool LRU_1_empty = buffer_q_LRU_1.IsEmpty();
		bool LRU_2_empty = buffer_q_LRU_2.IsEmpty();
		bool LRU_2_notifications_empty = LRU_2_notifications.IsEmpty();

		if (!LRU_1_empty) {
			struct desc_item temp_LRU_1 = *(buffer_q_LRU_1.WaitAndPop());
			int Flow_ID = temp_LRU_1.flow_id;
			int ByteCnt = temp_LRU_1.byte_cnt;
			int found = lru1->find(Flow_ID);
			if (found != -1) {
				int value = lru1->insertOld(Flow_ID, ByteCnt, found);
				if (value != 0) {
					LRU_2_notifications.Push(temp_LRU_1);
				}
			}
			else {
				lru1->insertNew(Flow_ID, ByteCnt);
			}
		}  
		
		//bool LRU_2_notifications_empty = LRU_2_notifications.IsEmpty();
		else if (LRU_1_empty && LRU_2_empty && LRU_2_notifications_empty) {
			if (WRITE) {
				//cout << "LRU_1 : " << buffer_q_LRU_1.IsEmpty() << " : " << buffer_q_LRU_2.IsEmpty() << " : " << LRU_2_notifications.IsEmpty() << endl;
				lru1->writeAllToDRAM();
				string str2 = "dram_accurate_value_";
				string str3 = "0";
				string str4 = ".txt";
				lru1->dram->writeToFile(str2+str3+str4);
			}
			break;
		}
    }
	return NULL;
}

void * LRU_LOGIC () {  
	//并行
    thread thread_LRU_2_LOGIC(LRU_2_LOGIC);
	thread thread_LRU_1_LOGIC(LRU_1_LOGIC);
	thread_LRU_2_LOGIC.join();
    thread_LRU_1_LOGIC.join();
	cout << "total : " << buffer_q_LRU_1.IsEmpty() << " : " << buffer_q_LRU_2.IsEmpty() << " : " << LRU_2_notifications.IsEmpty() << endl;
	//串行
	/*SmallLRU * lru1 = smalllru;
	BigLRU * lru2 = biglru;
	int Flow_ID = 0, ByteCnt = 0;
	for (long long i = 0;i < SCALE;i++) {
		struct desc_item temp = *(buffer_q_LRU_2.WaitAndPop());
		Flow_ID = temp.flow_id;
		ByteCnt = temp.byte_cnt;
		int found = 0;
		if ((found = lru2->find(Flow_ID)) != -1) {
			lru2->insertFromOut(Flow_ID, ByteCnt, found);
		}
		else {
			//cout << Flow_ID  << " : " << lru1->LRU_SIZE << endl;
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
	}*/
	
    return NULL;
}

int main() {
	//目前使用单线程进行读取
	ifstream readFile("D:\\report_2013.txt");
	string s = "";
	int i = 0;
	while (getline(readFile, s) && i < SCALE) {
		struct desc_item entry(s);
		buffer[i] = entry;
		//buffer_q_LRU_2.Push(entry);
		i++;
	}
	printf("ok\n");

	//init lru1, lru2
	smalllru = new SmallLRU();
	biglru = new BigLRU();
	smalllru->init(16 * 1024, 0x3fff, 14);
	biglru->init(112 * 1024, 0xffff, 16);
	
	thread thread_process_data(process_data);
	thread thread_LRU_LOGIC(LRU_LOGIC);  
	thread_process_data.join();

	clock_t start, finish;
	double totaltime;
	start = clock();  
    thread_LRU_LOGIC.join();
	finish = clock();
	totaltime = (double)(finish - start) / CLOCKS_PER_SEC;

	cout << totaltime << endl;
	double speed = SCALE / totaltime / 1000 / 1000;
	cout << speed << endl;
	delete[] smalllru;
	delete[] biglru;
    return 0;
}
