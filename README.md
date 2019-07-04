# case_cal
FARM：Fast Accurate peR-flow Measurement  
高速精确每流测量C++实现

## 更新日志
### 阶段工作
* 流量进入的LRU模块正确性验证
* LRU2大小问题以及Hash优化
### 2019年7月4日
1. 使用index[]数组来替换WRITE_TIMES逻辑
2. 加入线程对的速度测试，使用SPD_TEST开关

## 常量宏释义
* THREAD_NUM 线程数量
* CORE_NUM 核心数量
* SCALE 数据规模
* WRITE 是否将LRU写入文件
* SPD_TEST 是否编译线程速度测试内容
* START_PERCENT & END_PERCENT 单线程速度测试范围(以百分比计)
