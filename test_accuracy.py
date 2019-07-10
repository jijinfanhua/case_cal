
THREAD_NUM = 1
filename_DRAM = "dram_accurate_value_"
filename_SRAM = "sram_estimate_value_"
filename_real_trace = "real_trace_20000000.txt"

DRAM_dict = dict()
SRAM_dict = dict()
total_dict = dict()
REAL_dict = dict()

#load_real_trace:
count = 0
SCALE = 20000000

with open(filename_real_trace, 'r') as f_REAL:
    for REAL_temp in f_REAL.readlines():
        if count >= SCALE:
            break
        REAL_temp = REAL_temp.strip('\n')
        split_REAL_temp = REAL_temp.split(' ')
        key = split_REAL_temp[0]
        value = split_REAL_temp[1]
        if key in REAL_dict.keys():
            REAL_dict[key] += int(value)
        else:
            REAL_dict[key] = int(value)
        count += 1
print("load real trace successfully!")

#with open('real_trace_20000000.txt','w') as f_write:
#    for item in REAL_dict.keys():
#        f_write.write(item + ' ' + str(REAL_dict[item]) + '\n')



for i in range(0,THREAD_NUM):
    DRAM = filename_DRAM + str(i) + '.txt'
    SRAM = filename_SRAM + str(i) + '.txt'
    #print(DRAM,SRAM)
    # small flow
    with open(DRAM, 'r') as f_DRAM:
        for DRAM_temp in f_DRAM.readlines():
            DRAM_temp = DRAM_temp.strip('\n')
            #print(DRAM_temp)
            split_DRAM_temp = DRAM_temp.split('\t')
            #print(split_DRAM_temp[0], split_DRAM_temp[1])
            key = split_DRAM_temp[0]
            value = split_DRAM_temp[1]
            if key in DRAM_dict.keys():
                DRAM_dict[key] += int(value)
            else:
                DRAM_dict[key] = int(value)

            if key in total_dict.keys():
                total_dict[key] += int(value)
            else:
                total_dict[key] = int(value)


    # big flow
    with open(SRAM, 'r') as f_SRAM:
        for SRAM_temp in f_SRAM.readlines():
            SRAM_temp = SRAM_temp.strip('\n')
            split_SRAM_temp = SRAM_temp.split('\t')
            key = split_SRAM_temp[0]
            value = split_SRAM_temp[1]
            if key in SRAM_dict.keys():
                SRAM_dict[key] += int(value)
            else:
                SRAM_dict[key] = int(value)

            if key in total_dict.keys():
                total_dict[key] += int(value)
            else:
                total_dict[key] = int(value)

print("load test trace successfully!")
with open('DRAM_trace.txt', 'w') as f_write:
    for item in DRAM_dict.keys():
        f_write.write(item + ' ' + str(DRAM_dict[item]) + '\n')

with open('SRAM_trace.txt','w') as f_write:
    for item in SRAM_dict.keys():
        f_write.write(item + ' ' + str(SRAM_dict[item]) + '\n')

accur = 0.0
flow_number = 0
miss_flow_number = 0
miss_flow_byte = 0

accur_DRAM = 0.0
flow_number_DRAM = 0
miss_flow_number_DRAM = 0
miss_flow_byte_DRAM = 0

accur_SRAM = 0.0
flow_number_SRAM = 0
miss_flow_number_SRAM = 0
miss_flow_byte_SRAM = 0

for item in REAL_dict.keys():
    if item in total_dict.keys():
        flow_number += 1
        accur += 1 - float(abs(REAL_dict[item] - total_dict[item]))/ REAL_dict[item]
    else:
        #print(item, REAL_dict[item])
        miss_flow_number += 1
        miss_flow_byte += REAL_dict[item]
'''
for item in SRAM_dict.keys():
    if item in REAL_dict.keys():
        flow_number_SRAM += 1
        accur_SRAM += 1 - float(abs(REAL_dict[item] - SRAM_dict[item])) / REAL_dict[item]
    else:
        miss_flow_number_SRAM += 1
        miss_flow_number_SRAM += SRAM_dict[item]

for item in DRAM_dict.keys():
    if item in REAL_dict.keys():
        flow_number_DRAM += 1
        accur_DRAM += 1 - float(abs(REAL_dict[item] - DRAM_dict[item])) / REAL_dict[item]
    else:
        miss_flow_number_SRAM += 1
        miss_flow_number_SRAM += SRAM_dict[item]
'''

'''
print("SRAM accur : ", float(accur_SRAM)/flow_number_SRAM)
print("miss_flow_number : ", miss_flow_number_SRAM)
print("DRAM accur : ", float(accur_DRAM)/flow_number_DRAM)
print("miss_flow_number : ", miss_flow_number_DRAM)
'''

print("accur : ", float(accur)/flow_number)
print("miss_flow_number : ", miss_flow_number)
print("miss_flow_byte : ", miss_flow_byte)
