# case_cal
FARM：Fast Accurate peR-flow Measurement  
高速精确每流测量C++实现

## 更新日志
### 阶段工作
* 考虑使用向LRU2增加工作负担的办法减少LRU1Buffer的长度
  1. 直接在LRU2中插入的办法，可在10^-7级别增加精确度，但会带来较大速度损失
  2. 采用LRU2线程帮助LRU1线程处理的方法，亦会降低速度
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
