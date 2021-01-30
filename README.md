# case_cal
FARM：Fast Accurate peR-flow Measurement  
高速精确每流测量C++实现

## 更新日志
### 2019年7月12日
1. 删除计算模块，该模块只会降低速度
2. 加入packet count的计数
### 2019年7月8日
* 降低LRU1队列规模，采用双阈值控制LRU2逻辑协助LRU1进行数据处理。  
目前的方案是LRU2直接将此区间段本应PUSH入LRU1队列的数据在本循环中处理，即将数据entry当做大流数据录入SRAM中。  
测试结果速度和精度均未受到较大影响。
### 2019年7月7日
1. 修复速度测试BUG
### 2019年7月5日
1. 将速度测试改为时钟计时(原为CPU滴答数)
2. 加入volatile使能-O2/-O3(Czk)
3. WRITE_TIMES逻辑正确性验证(Czk)
### 2019年7月4日
1. 使用index[]数组来替换WRITE_TIMES逻辑
2. 加入线程对的速度测试，使用SPD_TEST开关
3. 去除一些冗余逻辑
4. make clean将删除程序运行中生成的txt文件

## 常量宏释义
* THREAD_NUM 线程数量
* CORE_NUM 核心数量
* SCALE 数据规模
* WRITE 是否将LRU写入文件
* SPD_TEST 是否编译线程速度测试内容
* START_PERCENT & END_PERCENT 单线程速度测试范围(以百分比计)
```
var a = 123;
s = c + d
```
