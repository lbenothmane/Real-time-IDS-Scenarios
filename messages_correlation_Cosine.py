from math import *
# from scipy.stats import pearsonr
import numpy as np
import time


pid_values=['254', '115','264','04A','010','050','111','113','117','118','119','121',
            '122','123','140','229','25C','14A','170','211','253','299','301','091','339','262','263',
            '346','417','342','344','40A','441','494','444','400','04B','340','405','350','4EC','65A','454','581']

cosine_similarity_1 = []

def get_ID(can_frames):
    canData = []
    for row in can_frames:
        row = str(row)
        record = {'PID': row[10:13]}
        canData.append(record)
        record = {}
    return canData


def cosine_similarity(temp):
    v1 = temp[0]
    v2 = temp[1]
    "compute cosine similarity of v1 to v2: (v1 dot v2)/{||v1||*||v2||)"
    sumxx, sumxy, sumyy = 0, 0, 0
    for i in range(len(v1)):
        x = v1[i]
        y = v2[i]
        sumxx += x * x
        sumyy += y * y
        sumxy += x * y
    result = sumxy / sqrt(sumxx * sumyy)
    return result

def map1(unique_pid):
    pid_data1 = []
    for i in range(0, 35):
        for j in range(0, 35):
            pid_data1.append((unique_pid[i]) + (unique_pid[j]))
    return pid_data1


def map2(key_data, data, low, high):
    map3 = dict.fromkeys(key_data, 0)
    for i in range(low, high, 1):
        key = (data[i]['PID']) + (data[i + 1]['PID'])
        if key in map3:
            map3[key] += 1
        else:
            map3[key] = 1
    result = []
    for key in key_data:
        result.append(map3[key])
    return result


def get_similarities(CanData, window):

    cosine_similarity_precedence = []
    pearson_similarity_precedence = []

    N_Slices = int(len(CanData) / window) - 4
    for i in range(N_Slices):
        l1 = window * i
        l2 = window * (i + 1)
        l3 = window * (i + 2)
        l4 = window * (i + 3)

        pid_data = map1(pid_values)
        v_1 = map2(pid_data, CanData, l1, l2)
        v_2 = map2(pid_data, CanData, l2, l3)
        v_3 = map2(pid_data, CanData, l3, l4)
        v_4 = abs(np.subtract(v_2, v_1))
        v_5 = abs(np.subtract(v_3, v_2))
        cosine_similarity_precedence.append(
            round(cosine_similarity([v_1, v_2]), 2))

    return cosine_similarity_precedence


def process_can_data(can_data):
    start = time.perf_counter()
    window = 100
    message_ID = get_ID(can_data)
    # print(message_ID)

    cosine_similar = get_similarities(message_ID, window)
    print(f"this the cosine similarity {cosine_similar}")
    #print(f"this the cosine average {np.average(cosine_similar)}")
    # Your code that you want to time
    end = time.perf_counter() - start
    print(f"this the end of pyhthon execution {end}")
    return 1
