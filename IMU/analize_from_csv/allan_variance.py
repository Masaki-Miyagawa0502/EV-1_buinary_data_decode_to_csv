import json
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colormaps
from pathlib import Path

###EV-1シリーズ及び、sbgのサンプリング周期の確認が必要、jsonファイルから読み込む###
ev1_jsonfile = open("ev1_setting.json", "r")
ev1_jsondata = json.load(ev1_jsonfile)
sbg_jsonfile = open("sbg_setting.json", "r")
sbg_jsondata = json.load(sbg_jsonfile)
#############################################################################


###アラン分散の平均時間を決める→平均時間とサンプリング周期から、取得サンプル数を決定###
###jsondataのsamplinデータはHzなので、取得サンプル数の計算は掛け算でよい###
average_time = [0.1, 1, 10, 60, 300, 600, 1800, 3600] ###単位は[s]
ev1_sampling_data_num = []
sbg_sampling_data_num = []
for i in range(len(average_time)):
    ev1_sampling_data_num.append(float(ev1_jsondata["sampling_time"]) * average_time[i])
    sbg_sampling_data_num.append(float(sbg_jsondata["sampling_time"]) * average_time[i])
################################################################################


###アラン分散を計算するデータ項目をjsonファイルから読み込む###
###EV-1とsbgでデータ項目名が異なる場合があるので、それぞれのcsvでどのようなデータ項目名でデコードされているか確認が必要###
allan_jsonfile = open("evaluate_key.json", "r")
allan_jsondata = json.load(allan_jsonfile)
allan_data_key = []
for key in allan_jsondata:
    allan_data_key.append(key)
#########################################################

###アラン分散を行いたいcsvファイルをjsonファイルから取得する###
ev1_csv_jsonfile = open("ev1_csv.json", "r")
ev1_csv_jsondata = json.load(ev1_csv_jsonfile)
ev1_csv_list = []
for key in ev1_csv_jsondata:
    ev1_csv_list.append(ev1_csv_jsondata[key])

# sbg_csv_jsonfile = open("sbg_csv.json", "r")
# sbg_csv_jsondata = json.load(sbg_csv_jsonfile)
# sbg_csv_list = []
# for key in sbg_csv_jsondata:
#     sbg_csv_list.append(sbg_csv_jsondata[key])
##########################################################

###結果(グラフ)の保存先###
output_jsonfile = open("output.json", "r")
output_jsondata = json.load(output_jsonfile)
for key in output_jsondata:
    output_dir = Path(output_jsondata[key])

###以下メインの処理###
####################

##allan_data_keyに格納されたデータ項目に対する平均値を格納する配列とアラン分散値を格納する配列をを作成###
###アラン分散用配列###
ev1_average = {}
sbg_average = {}
ev1_allan_varialance = {}
sbg_allan_varialance = {}
######################

for key in allan_jsondata:
    ev1_average[key] = []
    sbg_average[key] = []

for csv_num in range(len(ev1_csv_list)):
    ev1_allan_varialance[ev1_csv_list[csv_num]] = {}
    for key in allan_jsondata:
        ev1_allan_varialance[ev1_csv_list[csv_num]][key] = []

# for csv_num in range(len(sbg_csv_list)):
#     sbg_allan_varialance[sbg_csv_list[csv_num]] = {}
#     for key in allan_jsondata:
#         sbg_allan_varialance[sbg_csv_list[csv_num]][key] = []

###平均値計算用配列###
ev1_all_data_average= {}
sbg_all_data_average= {}
######################

for csv_num in range(len(ev1_csv_list)):
    ev1_all_data_average[ev1_csv_list[csv_num]] = {}
    for key in allan_jsondata:
        ev1_all_data_average[ev1_csv_list[csv_num]][key] = []

# for csv_num in range(len(sbg_csv_list)):
#     sbg_all_data_average[sbg_csv_list[csv_num]] = {}
#     for key in allan_jsondata:
#         sbg_all_data_average[ev1_csv_list[csv_num]][key] = []
###################################################################################################


###ev1_seriesのアラン分散計算###
if len(ev1_csv_list) > 0:
    for csv_num in range(len(ev1_csv_list)):
        with open(ev1_csv_list[csv_num], newline="") as file:
            csv_data = csv.DictReader(file)
            data_list = list(csv_data) ###何度も同じcsvデータを使用できるようにする###

            for key in allan_data_key:

                ###allan_data_keyに格納されたデータ項目とcsvのヘッダーが一致するデータのみ計算対象とする###
                ###取得サンプル数に応じて平均値を求めて、平均値を格納する配列に追加###
                # ###平均値を求める途中でサンプル数が足りなくなったら、平均値計算をやめ、アラン分散を求める処理に進む###
                if key not in csv_data.fieldnames:
                    continue

                ###1列分のデータを取得サンプル数に応じて平均値を求め、平均値配列に格納###
                for data_count in range(len(ev1_sampling_data_num)):
                    cycle_num = int(ev1_sampling_data_num[data_count])
                    current_cycle_num = 0
                    value = 0 
                    average = 0
                    average_sum = 0
                    allan_value = 0
                    ev1_average[key].clear()

                    for row_data in data_list:
                        if row_data[key] == "" :
                            break

                        value += float(row_data[key])
                        current_cycle_num += 1

                        if current_cycle_num == cycle_num:
                            average = value / cycle_num
                            ev1_average[key].append(average)
                            current_cycle_num = 0
                            value = 0

                    ###アラン分散計算###
                    if len(ev1_average[key]) >= 2:
                        for ave_value_index in range(len(ev1_average[key]) - 1):
                            average_sum += (
                                ev1_average[key][ave_value_index]
                                - ev1_average[key][ave_value_index + 1]
                            ) ** 2

                        allan_value = 0.5 * average_sum / (len(ev1_average[key]) - 1)
                    else:
                        allan_value = float("nan")

                    ev1_allan_varialance[ev1_csv_list[csv_num]][key].append(allan_value)

####################################################################

###sbgのアラン分散計算###
# if len(sbg_csv_list) > 0:
#     for csv_num in range(len(sbg_csv_list)):
#         with open(sbg_csv_list[csv_num], newline="") as file:
#             csv_data = csv.DictReader(file)
#             data_list = list(csv_data) ###何度も同じcsvデータを使用できるようにする###

#             for key in allan_data_key:

#                 ###allan_data_keyに格納されたデータ項目とcsvのヘッダーが一致するデータのみ計算対象とする###
#                 ###取得サンプル数に応じて平均値を求めて、平均値を格納する配列に追加###
#                 # ###平均値を求める途中でサンプル数が足りなくなったら、平均値計算をやめ、アラン分散を求める処理に進む###
#                 if key not in csv_data.fieldnames:
#                     continue

#                 ###1列分のデータを取得サンプル数に応じて平均値を求め、平均値配列に格納###
#                 for data_count in range(len(sbg_sampling_data_num)):
#                     cycle_num = int(sbg_sampling_data_num[data_count])
#                     current_cycle_num = 0
#                     value = 0 
#                     average = 0
#                     average_sum = 0
#                     allan_value = 0
#                     sbg_average[key].clear()

#                     for row_data in data_list:
#                         if row_data[key] == "" :
#                             break
        
#                         value += float(row_data[key])
#                         current_cycle_num += 1

#                         if current_cycle_num == cycle_num:
#                             average = value / cycle_num
#                             sbg_average[key].append(average)
#                             current_cycle_num = 0
#                             value = 0

#                     ###アラン分散計算###
#                     if len(sbg_average[key]) >= 2:
#                         for ave_value_index in range(len(sbg_average[key]) - 1):
#                             average_sum += (
#                                 sbg_average[key][ave_value_index]
#                                 - sbg_average[key][ave_value_index + 1]
#                             ) ** 2

#                         allan_value = 0.5 * average_sum / (len(sbg_average[key]) - 1)
#                     else:
#                         allan_value = float("nan")

#                     sbg_allan_varialance[sbg_csv_list[csv_num]][key].append(allan_value)
####################################################################

###ev1_seriesの平均値計算###
if len(ev1_csv_list) > 0:
    for csv_num in range(len(ev1_csv_list)):
        with open(ev1_csv_list[csv_num], newline="") as file:
            csv_data = csv.DictReader(file)
            data_list = list(csv_data) ###何度も同じcsvデータを使用できるようにする###

            for key in allan_data_key:

                if key not in csv_data.fieldnames:
                    continue

                data_len = 0
                value = 0 
                average = 0

                for row_data in data_list:
                    if row_data[key] == "" :
                        break
                    else:
                        data_len += 1
                        value += float(row_data[key])
                
                average = value / data_len
                ev1_all_data_average[ev1_csv_list[csv_num]][key].append(average)

###sbgの平均値計算###
# if len(sbg_csv_list) > 0:
#     for csv_num in range(len(sbg_csv_list)):
#         with open(sbg_csv_list[csv_num], newline="") as file:
#             csv_data = csv.DictReader(file)
#             data_list = list(csv_data) ###何度も同じcsvデータを使用できるようにする###

#             for key in allan_data_key:

#                 if key not in csv_data.fieldnames:
#                     continue

#                 data_len = 0
#                 value = 0 
#                 average = 0

#                 for row_data in data_list:
#                     if row_data[key] == "" :
#                         average = value / data_len
#                         sbg_all_data_average[sbg_csv_list[csv_num]][key].append(average)
#                         break
#                     else:
#                         data_len += 1
#                         value += float(row_data[key])



###アラン分散をプロット###
marker_list = ["s", "o", "^", "v", "D", "x", "*"]
for key in allan_data_key:
    plt.figure()  # 項目ごとに別の図を作る
    x = np.array(average_time)
    for csv_num in range(len(ev1_csv_list)):
        y = np.array(ev1_allan_varialance[ev1_csv_list[csv_num]][key])
        
        plt.plot(x, y, color=plt.cm.tab10(csv_num), label=f"EV-1_N{csv_num + 1}", marker=marker_list[csv_num], linestyle = "-")

    # for csv_num in range(len(sbg_csv_list)):
    #     y = np.array(sbg_allan_varialance[sbg_csv_list[csv_num]][key])
    #     plt.plot(x, y, color=plt.cm.tab10(csv_num), label=f"sbg_N{csv_num + 1}", marker=marker_list[csv_num], linestyle="--")

    plt.xlabel("average_time [s]")
    if key in ("ax", "ay", "az"):
        plt.ylabel("allan_varialance [m/s^2]")
    else:
        plt.ylabel("allan_varialance [°/s^2]")

    plt.xscale("log")
    plt.yscale("log")
    plt.title(f"{key} Allan variance: EV-1 and SBG")
    plt.legend()
    plt.grid(True, which="both")
    plt.savefig(output_dir / f"{key} Allan variance: EV-1 and SBG.png", dpi=300, bbox_inches="tight")
    plt.show()

##平均値をプロット###
for key in allan_data_key:
    plt.figure()  # 項目ごとに別の図を作る

    if len(ev1_csv_list) == len(sbg_csv_list):
        x = np.arange(1, len(ev1_csv_list))
    else:
        print("input_csv_num ev1 and sbg should be same")
        
    for csv_num in range(len(ev1_csv_list)):
        y = np.array(ev1_all_data_average[ev1_csv_list[csv_num]][key])
        
        plt.plot(x, y, color=plt.cm.tab10(csv_num), label=f"EV-1_N{csv_num + 1}", marker=marker_list[csv_num], linestyle = "-")

    # for csv_num in range(len(sbg_csv_list)):
    #     y = np.array(sbg_all_data_average[sbg_csv_list[csv_num]][key])
    #     plt.plot(x, y, color=plt.cm.tab10(csv_num), label=f"sbg_N{csv_num + 1}", marker=marker_list[csv_num], linestyle="--")

    plt.xlabel("average_num")
    if key in ("ax", "ay", "az"):
        plt.ylabel("average_value [m/s^2]")
    else:
        plt.ylabel("average_value [°/s]")

    plt.title(f"{key} average: EV-1 and SBG")
    plt.legend()
    plt.grid(True, which="both")
    plt.savefig(output_dir / f"{key} all_data_average: EV-1 and SBG.png", dpi=300, bbox_inches="tight")
    plt.show()
