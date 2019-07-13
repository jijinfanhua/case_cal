

def generate_real_trace(scale, filename):
    SCALE = scale
    cnt = 0
    byte_dict = {}
    pkt_dict = {}

    f = open('report_2013.txt', 'r')
    while 1:
        line = f.readline()
        if (not line) or cnt >= SCALE:
            break
        arr = line.split()
        flowid, bytecnt = int(arr[0]), int(arr[1])
        byte_dict[flowid] = byte_dict.get(flowid, 0) + bytecnt
        pkt_dict[flowid] = pkt_dict.get(flowid, 0) + 1
        cnt += 1
    f.close()
    all = 0
    with open(filename, 'w') as f:
        for key, value in byte_dict.items():
            f.write('{}\t{}\t{}\n'.format(str(key), str(value), str(pkt_dict[key])))
            all += pkt_dict[key]
    print('generate ', all, ' packets trace in ', filename)

def load_real_trace(filename_real_trace):
    byte_sum, pkt_sum = 0, 0
    REAL_byte_dict, REAL_pkt_dict = {}, {}

    with open(filename_real_trace, 'r') as f_REAL:
        for REAL_temp in f_REAL.readlines():
            split_REAL_temp = REAL_temp.split()
            flow_id, byte_value, pkt_value = int(split_REAL_temp[0]), int(split_REAL_temp[1]), int(split_REAL_temp[2])
            REAL_byte_dict[flow_id] = REAL_byte_dict.get(flow_id, 0) + byte_value
            REAL_pkt_dict[flow_id] = REAL_pkt_dict.get(flow_id, 0) + pkt_value
            byte_sum += byte_value
            pkt_sum += pkt_value
    print("load real trace successfully, all bytes: ", byte_sum, ' all packets: ', pkt_sum)
    return REAL_byte_dict, REAL_pkt_dict

def load_cal_trace(thread_num, filename_DRAM, filename_SRAM):
    all_byte, all_pkt = 0, 0
    DRAM_byte_dict, DRAM_pkt_dict, SRAM_byte_dict, SRAM_pkt_dict, total_byte_dict, total_pkt_dict = {}, {}, {}, {}, {}, {}
    for i in range(0, THREAD_NUM):
        DRAM = filename_DRAM + str(i) + '.txt'
        SRAM = filename_SRAM + str(i) + '.txt'

        with open(DRAM, 'r') as f_DRAM:
            for DRAM_temp in f_DRAM.readlines():
                split_DRAM_temp = DRAM_temp.split()
                flow_id, byte_value, pkt_value = int(split_DRAM_temp[0]), int(split_DRAM_temp[1]), int(split_DRAM_temp[2])

                DRAM_byte_dict[flow_id] = DRAM_byte_dict.get(flow_id, 0) + byte_value
                DRAM_pkt_dict[flow_id] = DRAM_pkt_dict.get(flow_id, 0) + pkt_value

                total_byte_dict[flow_id] = total_byte_dict.get(flow_id, 0) + byte_value
                total_pkt_dict[flow_id] = total_pkt_dict.get(flow_id, 0) + pkt_value

                all_pkt += pkt_value
                all_byte += byte_value

        with open(SRAM, 'r') as f_SRAM:
            for SRAM_temp in f_SRAM.readlines():
                # SRAM_temp = SRAM_temp.strip('\n')
                # split_SRAM_temp = SRAM_temp.split('\t')
                split_SRAM_temp = SRAM_temp.split()
                flow_id, byte_value, pkt_value = int(split_SRAM_temp[0]), int(split_SRAM_temp[1]), int(
                    split_SRAM_temp[2])

                SRAM_byte_dict[flow_id] = SRAM_byte_dict.get(flow_id, 0) + byte_value
                SRAM_pkt_dict[flow_id] = SRAM_pkt_dict.get(flow_id, 0) + pkt_value

                total_byte_dict[flow_id] = total_byte_dict.get(flow_id, 0) + byte_value
                total_pkt_dict[flow_id] = total_pkt_dict.get(flow_id, 0) + pkt_value

                all_pkt += pkt_value
                all_byte += byte_value
    print("load cal trace successfully, all bytes: ", all_byte, ' all packets: ', all_pkt)
    return DRAM_byte_dict, DRAM_pkt_dict, SRAM_byte_dict, SRAM_pkt_dict, total_byte_dict, total_pkt_dict

def write_cal_to_file(SRAM_byte_dict, SRAM_pkt_dict, DRAM_byte_dict, DRAM_pkt_dict,total_byte_dict, total_pkt_dict):
    # total_byte_dict = dict(sorted(total_byte_dict.items(), key=lambda item: item[0]))
    with open('total_trace.txt', 'w') as f_write:
        for item in total_byte_dict.keys():
            f_write.write(str(item) + '\t' + str(total_byte_dict[item]) + '\t' + str(total_pkt_dict[item]) + '\n')

    # SRAM_byte_dict = dict(sorted(SRAM_byte_dict.items(), key=lambda item: item[0]))
    with open('SRAM_trace.txt', 'w') as f_write:
        for item in SRAM_byte_dict.keys():
            f_write.write(str(item) + '\t' + str(SRAM_byte_dict[item]) + '\t' + str(SRAM_pkt_dict[item]) + '\n')

    # DRAM_byte_dict = dict(sorted(DRAM_byte_dict.items(), key=lambda item: item[0]))
    with open('DRAM_trace.txt', 'w') as f_write:
        for item in DRAM_byte_dict.keys():
            f_write.write(str(item) + '\t' + str(DRAM_byte_dict[item]) + '\t' + str(DRAM_pkt_dict[item]) + '\n')

def cmp_real_and_cal(SRAM_byte_dict, SRAM_pkt_dict, DRAM_byte_dict, DRAM_pkt_dict, REAL_byte_dict, REAL_pkt_dict):
    sram_miss_flow_num, sram_miss_pkt_num, sram_miss_byte_num, dram_miss_flow_num, dram_miss_pkt_num, dram_miss_byte_num = 0, 0, 0, 0, 0, 0
    with open('sram_cmp.txt', 'w') as f:
        for item in SRAM_byte_dict.keys():
            if item in REAL_byte_dict.keys():
                f.write(str(item) + '\t' + str(SRAM_byte_dict[item]) + '\t' + str(REAL_byte_dict[item]) + '\t' + str(SRAM_pkt_dict[item]) + '\t' + str(REAL_pkt_dict[item]) + '\n')
    print('generate sram byte and packets compare in sram_cmp.txt!')

    with open('dram_cmp.txt', 'w') as f:
        for item in DRAM_byte_dict.keys():
            if item in REAL_byte_dict.keys():
                f.write(str(item) + '\t' + str(DRAM_byte_dict[item]) + '\t' + str(REAL_byte_dict[item]) + '\t' + str(DRAM_pkt_dict[item]) + '\t' + str(REAL_pkt_dict[item]) + '\n')
    print('generate dram byte and packets compare in dram_cmp.txt!')

if __name__ == '__main__':
    THREAD_NUM = 1
    SCALE = 20000000
    filename_DRAM = "dram_accurate_value_"
    filename_SRAM = "sram_estimate_value_"
    filename_real_trace = "real_trace_20000000_new.txt"

    # generate_real_trace(SCALE, filename_real_trace) # use this to generate real trace
    REAL_byte_dict, REAL_pkt_dict = load_real_trace(filename_real_trace)
    DRAM_byte_dict, DRAM_pkt_dict, SRAM_byte_dict, SRAM_pkt_dict, total_byte_dict, total_pkt_dict = load_cal_trace(THREAD_NUM, filename_DRAM, filename_SRAM)

    total_byte_dict = dict(sorted(total_byte_dict.items(), key=lambda item: item[0]))
    SRAM_byte_dict = dict(sorted(SRAM_byte_dict.items(), key=lambda item: item[0]))
    DRAM_byte_dict = dict(sorted(DRAM_byte_dict.items(), key=lambda item: item[0]))

    write_cal_to_file(SRAM_byte_dict, SRAM_pkt_dict, DRAM_byte_dict, DRAM_pkt_dict,total_byte_dict, total_pkt_dict)
    cmp_real_and_cal(SRAM_byte_dict, SRAM_pkt_dict, DRAM_byte_dict, DRAM_pkt_dict, REAL_byte_dict, REAL_pkt_dict)

    accur = 0.0
    flow_number = 0
    miss_flow_number = 0
    miss_flow_byte = 0

    pkt_number = 0

    for item in REAL_byte_dict.keys():
        if item in total_byte_dict.keys():
            flow_number += 1
            accur += 1 - float(abs(REAL_byte_dict[item] - total_byte_dict[item])) / REAL_byte_dict[item]
        else:
            # print(item, REAL_dict[item])
            miss_flow_number += 1
            miss_flow_byte += REAL_byte_dict[item]

    for item in REAL_pkt_dict.keys():
        if item in total_pkt_dict.keys():
            pkt_number += total_pkt_dict[item]

    print("bytes accur : ", float(accur) / flow_number)
    print("miss_flow_number : ", miss_flow_number)
    print("miss_flow_byte : ", miss_flow_byte)
    print("packet number : ", pkt_number)
    print("packets accur : ", pkt_number * 1.0 / SCALE)
