
#include <iostream>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <future>
#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#ifdef _WIN32
#include <Windows.h>
#endif

#include <time.h>

#include "big_lru.h"
#include "small_lru.h"
#include "Safe_Queue.h"

#define THREAD_NUM 1
#define CORE_NUM 4
#define SCALE 20000000
#define WRITE true
#define SPD_TEST true
#define START_PERCENT 0.25
#define END_PERCENT 0.75
using namespace std;
using namespace chrono;

SmallLRU *smalllru[THREAD_NUM];
BigLRU *biglru[THREAD_NUM];
volatile int index1[THREAD_NUM] , index2[THREAD_NUM];
#if SPD_TEST
volatile double dur[THREAD_NUM];
bool startFlag[THREAD_NUM], endFlag[THREAD_NUM];
time_point<system_clock> start[THREAD_NUM];
time_point<system_clock> finish[THREAD_NUM];
#endif

void SplitString(const string &s, vector<string> &v, const string &c) {
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

QUEUE_DATA<desc_item> *buffer_q_LRU_1[THREAD_NUM];
QUEUE_DATA<desc_item> *buffer_q_LRU_2[THREAD_NUM];
QUEUE_DATA<desc_item> *LRU_2_notifications[THREAD_NUM];

typedef struct LRU_Thread_Arg {
    int LRU_index;
} LRU_Thread_Arg;




void *LRU_1_LOGIC(LRU_Thread_Arg *arg) {
    SmallLRU *lru1 = smalllru[arg->LRU_index];
    struct desc_item temp_LRU_1;
    int flag_LRU_1 = 0;
    case_flowid_t Flow_ID = 0;
    case_bytecnt_t ByteCnt = 0;
    int found = 0;
    case_bytecnt_t value = 0;
    while (index1[arg->LRU_index] + index2[arg->LRU_index] < SCALE/THREAD_NUM-1) {
        flag_LRU_1 = buffer_q_LRU_1[arg->LRU_index]->pop_data(&temp_LRU_1);
        if (flag_LRU_1 == 0) {
            Flow_ID = temp_LRU_1.flow_id;
            ByteCnt = temp_LRU_1.byte_cnt;
            found = lru1->find(Flow_ID);
            if (found != -1) {
                value = lru1->insertOld(Flow_ID, ByteCnt, found);
                if (value != 0) {
                    temp_LRU_1.byte_cnt = value;
                    //buffer_q_LRU_2[arg->LRU_index]->push_data(temp_LRU_1);
                    LRU_2_notifications[arg->LRU_index]->push_data(temp_LRU_1);
                    /*int length = LRU_2_notifications[arg->LRU_index]->queue_size();
                    if (length > LRU_2_notifications_max_length)
                        LRU_2_notifications_max_length = length;*/
                } else {
                    index1[arg->LRU_index]++;
                }
            } else {
                lru1->insertNew(Flow_ID, ByteCnt);
                index1[arg->LRU_index]++;
            }
        }
    }

#ifdef _WIN32
    cout << "LRU_1_LOGIC_end : " << GetCurrentTime() << endl;
#endif

    if (WRITE) {
        lru1->writeAllToDRAM();
        string str2 = "dram_accurate_value_";
        string str3 = std::to_string(arg->LRU_index);
        string str4 = ".txt";
        lru1->dram->writeToFile(str2 + str3 + str4);
    }
#if PRINT_CAL
    lru1->dram->write_count_to_file();
#endif
    return nullptr;
}
void *LRU_2_LOGIC(LRU_Thread_Arg *arg) {
#if SPD_TEST
	startFlag[arg->LRU_index] = true;
	endFlag[arg->LRU_index] = true;
#endif
    BigLRU *lru2 = biglru[arg->LRU_index];
    struct desc_item temp_LRU_2;
    case_flowid_t Flow_ID = 0;
    case_bytecnt_t ByteCnt = 0;
    int found = 0;
    //如果要写文件要保证运行足够长的时间
    //cout << SCALE/THREAD_NUM*WRITE_TIMES << endl;
    while (index1[arg->LRU_index] + index2[arg->LRU_index] < SCALE/THREAD_NUM-1) {
#if SPD_TEST
		if (startFlag[arg->LRU_index] && (index1[arg->LRU_index] + index2[arg->LRU_index] > SCALE / THREAD_NUM * START_PERCENT)) {
			start[arg->LRU_index] = system_clock::now();
			startFlag[arg->LRU_index] = false;
		}
		if (endFlag[arg->LRU_index] && (index1[arg->LRU_index] + index2[arg->LRU_index] > SCALE / THREAD_NUM * END_PERCENT)) {
			finish[arg->LRU_index] = system_clock::now();
			duration<double> diff = finish[arg->LRU_index] - start[arg->LRU_index];
			dur[arg->LRU_index] = diff.count();
			endFlag[arg->LRU_index] = false;
		}
#endif
        if (buffer_q_LRU_2[arg->LRU_index]->pop_data(&temp_LRU_2) == 0) {
            Flow_ID = temp_LRU_2.flow_id;
            ByteCnt = temp_LRU_2.byte_cnt;

            if ((found = lru2->find(Flow_ID)) != -1) {
                lru2->insertFromOut(Flow_ID, ByteCnt, found);
                index2[arg->LRU_index] ++;
            } else if (ByteCnt > SMALL_BYTE_THRES) {
                lru2->insertFromSmallLRU(Flow_ID, ByteCnt);
                index2[arg->LRU_index] ++;
            } else {
                buffer_q_LRU_1[arg->LRU_index]->push_data(temp_LRU_2);
            }
        }

        if (LRU_2_notifications[arg->LRU_index]->pop_data(&temp_LRU_2) == 0) {
            Flow_ID = temp_LRU_2.flow_id;
            ByteCnt = temp_LRU_2.byte_cnt;
            if ((found = lru2->find(Flow_ID)) != -1) {
                lru2->insertFromOut(Flow_ID, ByteCnt, found);
            } else {
                lru2->insertFromSmallLRU(Flow_ID, ByteCnt);
            }
            index2[arg->LRU_index] ++;
        }
    }

    //std::cout << buffer_q_LRU_1[arg->LRU_index]->queue_size() << endl;
    //std::cout << buffer_q_LRU_2[arg->LRU_index]->queue_size() << endl;
    //std::cout << LRU_2_notifications[arg->LRU_index]->queue_size() << endl;
    

#ifdef _WIN32
    cout << "LRU_2_LOGIC_end : " << GetCurrentTime() << endl;
#endif
    //cout << "buffer_q_LRU_1 : " << LRU_1_max_length << endl;
    if (WRITE) {
        lru2->writeAllToSRAM();
        string str1 = "sram_estimate_value_";
        string str3 = std::to_string(arg->LRU_index);
        string str4 = ".txt";
        lru2->sram->writeToFile(str1 + str3 + str4);
    }

//cout << PRINT_CAL << endl;
#if PRINT_CAL
    lru2->sram->cal_table_write_to_file();
    lru2->sram->write_count_to_file();
    lru2->sram->off_cal_file();
#endif
    return nullptr;
}

void *LRU_LOGIC(LRU_Thread_Arg *arg) {
    //并行
    thread thread_LRU_1_LOGIC(LRU_1_LOGIC, arg);
    thread thread_LRU_2_LOGIC(LRU_2_LOGIC, arg);
    thread_LRU_1_LOGIC.join();
    thread_LRU_2_LOGIC.join();
    return nullptr;
}

int main() {
    thread LRU_threads[THREAD_NUM];
    LRU_Thread_Arg LRU_args[THREAD_NUM];
    /*DWORD_PTR thread_mask[8];
    thread_mask[0] = 0x01;
    for (int i = 1;i < 8;i++) {
        thread_mask[i] = thread_mask[i] * 2;
    }*/
    for (int i = 0; i < THREAD_NUM; i++) {
        index1[i]=index2[i]=0;
        LRU_args[i].LRU_index = i;
        buffer_q_LRU_1[i] = new QUEUE_DATA<desc_item>(SCALE);
        buffer_q_LRU_2[i] = new QUEUE_DATA<desc_item>(SCALE);
        LRU_2_notifications[i] = new QUEUE_DATA<desc_item>(1024);
        smalllru[i] = new SmallLRU();
        biglru[i] = new BigLRU();
        smalllru[i]->init(16 * 1024, 0x3fff, 14);
        biglru[i]->init(112 * 1024, 0xffff, 16);
    }
    //目前使用单线程进行读取
    ifstream readFile("report_2013.txt");
    //ifstream readFile("test_trace.txt");
    string s = "";
    int i = 0;
    while (getline(readFile, s) && i < SCALE) {
        struct desc_item entry(s);
        //cout << i%THREAD_NUM << endl;
        buffer_q_LRU_2[i % THREAD_NUM]->push_data(entry);
        i++;
    }
    printf("ok\n");

    for (int i = 0; i < THREAD_NUM; i++) {
        //cout << i << " : " << buffer_q_LRU_2[i]->queue_size() << endl;
        LRU_threads[i] = thread(LRU_LOGIC, &LRU_args[i]);
        //SetThreadAffinityMask(LRU_threads[i].native_handle(),thread_mask[i%CORE_NUM]);
    }
    double totaltime;
    auto start = system_clock::now();
    for (int i = 0; i < THREAD_NUM; i++) {
        LRU_threads[i].join();
    }
    auto finish = system_clock::now();
    duration<double> diff = finish - start;
    totaltime = diff.count();

    std::cout << totaltime << endl;
    double speed = SCALE / totaltime / 1000 / 1000;
    std::cout << speed << endl;

#if SPD_TEST
	for (int i = 0; i < THREAD_NUM; i++)
	{
		std::cout << "Thread" << i << ": "<< (SCALE/THREAD_NUM*(END_PERCENT-START_PERCENT))/dur[i]/1000000<<" Mpps"<<endl;
		std::cout<<"index1["<<i<<"]:"<<index1[i]<<"  index2["<<i<<"]:"<<index2[i]<<endl;
	}
#endif

    for (int i = 0; i < THREAD_NUM; i++) {
        delete smalllru[i];
        delete biglru[i];
        delete buffer_q_LRU_1[i];
        delete buffer_q_LRU_2[i];
    }
#ifdef _WIN32
	system("pause");
#endif
    return 0;
}
