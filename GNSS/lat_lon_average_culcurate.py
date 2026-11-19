import json
import csv
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

### ev1及びsbgのGNSS信号ステータス名を設定 ###
ev1_signal_status = "fix_mode"
sbg_signal_status = "position_type"

### 計算したいデータ項目を抽出する ###
with open("lat_lon_evaluate_key.json", "r", encoding="utf-8") as data_jsonfile:
    data_jsondata = json.load(data_jsonfile)
data = list(data_jsondata.values())
lat_key = next((key for key in data if key.lower().startswith("lat")), None)
lon_key = next((key for key in data if key.lower().startswith("lon")), None)
if lat_key is None or lon_key is None:
    raise ValueError("lat_lon_evaluate_key.json must include latitude and longitude fields.")

### EV1及びSBGのcsvファイルを読み込む ###
with open("ev1_csv.json", "r", encoding="utf-8") as ev1_csv_jsonfile:
    ev1_csv = list(json.load(ev1_csv_jsonfile).values())
with open("sbg_csv.json", "r", encoding="utf-8") as sbg_csv_jsonfile:
    sbg_csv = list(json.load(sbg_csv_jsonfile).values())
with open("sbg_rtk_csv.json", "r", encoding="utf-8") as sbg_rtk_csv_jsonfile:
    sbg_rtk_csv = list(json.load(sbg_rtk_csv_jsonfile).values())

### 結果(グラフ)の保存先 ###
with open("output.json", "r", encoding="utf-8") as output_jsonfile:
    output_jsondata = json.load(output_jsonfile)
output_dir = Path(next(iter(output_jsondata.values())))

### EV1及びSBGの平均値、ばらつきを格納する配列を作成 ###
ev1_average = {}
ev1_1signa_variance = {}
ev1_3sigma_variance = {}
ev1_ave_sat_num = []
ev1_max_sat_num = []
ev1_min_sat_num = []
for csv_num in range(len(ev1_csv)):
    ev1_average[csv_num] = {}
    ev1_1signa_variance[csv_num] = {}
    ev1_3sigma_variance[csv_num] = {}
    for key in data:
        if key != "svnum":
            ev1_average[csv_num][key] = []
            ev1_1signa_variance[csv_num][key] = []
            ev1_3sigma_variance[csv_num][key] = []

sbg_average = {}
sbg_1sigma_variance = {}
sbg_3sigma_variance = {}
sbg_ave_sat_num = []
sbg_max_sat_num = []
sbg_min_sat_num = []
for csv_num in range(len(sbg_csv)):
    sbg_average[csv_num] = {}
    sbg_1sigma_variance[csv_num] = {}
    sbg_3sigma_variance[csv_num] = {}
    for key in data:
        if key != "svnum":
            sbg_average[csv_num][key] = []
            sbg_1sigma_variance[csv_num][key] = []
            sbg_3sigma_variance[csv_num][key] = []

sbg_rtk_average = {}
sbg_rtk_1sigma_variance = {}
sbg_rtk_3sigma_variance = {}
ev1_plot_lat = [[] for _ in ev1_csv]
ev1_plot_lon = [[] for _ in ev1_csv]
sbg_plot_lat = [[] for _ in sbg_csv]
sbg_plot_lon = [[] for _ in sbg_csv]
sbg_rtk_plot_lat = [[] for _ in sbg_rtk_csv]
sbg_rtk_plot_lon = [[] for _ in sbg_rtk_csv]
for csv_num in range(len(sbg_rtk_csv)):
    sbg_rtk_average[csv_num] = {}
    sbg_rtk_1sigma_variance[csv_num] = {}
    sbg_rtk_3sigma_variance[csv_num] = {}
    for key in data:
        if key != "svnum":
            sbg_rtk_average[csv_num][key] = []
            sbg_rtk_1sigma_variance[csv_num][key] = []
            sbg_rtk_3sigma_variance[csv_num][key] = []

### EV1のCSVを読み込む ###
for csv_num in range(len(ev1_csv)):
    with open(ev1_csv[csv_num], newline="", encoding="utf-8-sig") as file:
        csv_data = csv.DictReader(file)
        data_list = list(csv_data)
        fieldnames = csv_data.fieldnames or []

        for row_data in data_list:
            try:
                if float(row_data[ev1_signal_status]) == 2:
                    ev1_plot_lat[csv_num].append(float(row_data[lat_key]))
                    ev1_plot_lon[csv_num].append(float(row_data[lon_key]))
            except (KeyError, TypeError, ValueError):
                continue

        for key in data:
            if key not in fieldnames:
                continue
            ev1_element = []
            for row_data in data_list:
                try:
                    status = float(row_data[ev1_signal_status])
                    value = float(row_data[key])
                except (KeyError, TypeError, ValueError):
                    continue
                if status == 2:
                    ev1_element.append(value)

            if not ev1_element:
                continue
            if key == "svnum":
                ev1_ave_sat_num.append(float(np.average(ev1_element)))
                ev1_max_sat_num.append(float(np.max(ev1_element)))
                ev1_min_sat_num.append(float(np.min(ev1_element)))
            else:
                mean = float(np.average(ev1_element))
                std1 = float(np.std(ev1_element))
                ev1_average[csv_num][key].append(mean)
                ev1_1signa_variance[csv_num][key].append(std1)
                ev1_3sigma_variance[csv_num][key].append(3 * std1)

### SBG単独測位のCSVを読み込む ###
for csv_num in range(len(sbg_csv)):
    with open(sbg_csv[csv_num], newline="", encoding="utf-8-sig") as file:
        csv_data = csv.DictReader(file)
        data_list = list(csv_data)
        fieldnames = csv_data.fieldnames or []

        for row_data in data_list:
            try:
                if float(row_data[sbg_signal_status]) == 2:
                    sbg_plot_lat[csv_num].append(float(row_data[lat_key]))
                    sbg_plot_lon[csv_num].append(float(row_data[lon_key]))
            except (KeyError, TypeError, ValueError):
                continue

        for key in data:
            if key not in fieldnames:
                continue
            sbg_element = []
            for row_data in data_list:
                try:
                    status = float(row_data[sbg_signal_status])
                    value = float(row_data[key])
                except (KeyError, TypeError, ValueError):
                    continue
                if status == 2:
                    sbg_element.append(value)

            if not sbg_element:
                continue
            if key == "svnum":
                sbg_ave_sat_num.append(float(np.average(sbg_element)))
                sbg_max_sat_num.append(float(np.max(sbg_element)))
                sbg_min_sat_num.append(float(np.min(sbg_element)))
            else:
                mean = float(np.average(sbg_element))
                std1 = float(np.std(sbg_element))
                sbg_average[csv_num][key].append(mean)
                sbg_1sigma_variance[csv_num][key].append(std1)
                sbg_3sigma_variance[csv_num][key].append(3 * std1)

### SBG RTKのCSVを読み込む ###
for csv_num in range(len(sbg_rtk_csv)):
    with open(sbg_rtk_csv[csv_num], newline="", encoding="utf-8-sig") as file:
        csv_data = csv.DictReader(file)
        data_list = list(csv_data)
        fieldnames = csv_data.fieldnames or []

        for row_data in data_list:
            try:
                if float(row_data[sbg_signal_status]) == 7:
                    sbg_rtk_plot_lat[csv_num].append(float(row_data[lat_key]))
                    sbg_rtk_plot_lon[csv_num].append(float(row_data[lon_key]))
            except (KeyError, TypeError, ValueError):
                continue

        for key in data:
            if key == "svnum" or key not in fieldnames:
                continue
            sbg_element = []
            for row_data in data_list:
                try:
                    status = float(row_data[sbg_signal_status])
                    value = float(row_data[key])
                except (KeyError, TypeError, ValueError):
                    continue
                if status == 7:
                    sbg_element.append(value)

            if not sbg_element:
                continue
            mean = float(np.average(sbg_element))
            std1 = float(np.std(sbg_element))
            sbg_rtk_average[csv_num][key].append(mean)
            sbg_rtk_1sigma_variance[csv_num][key].append(std1)
            sbg_rtk_3sigma_variance[csv_num][key].append(3 * std1)

### 表の列 ###
columns = ["ref_lat_average", "ref_lat_1σ", "ref_lat_3σ", "ref_lon_average", "ref_lon_1σ", "ref_lon_3σ", "lat_average", "lat_1σ", "lat_3σ", "lon_average", "lon_1σ", "lon_3σ", "get_satellite_num_ave", "get_satellite_num_max", "get_satellite_num_min"]

### EV1の表作成 ###
if len(ev1_csv) > 0:
    ev1_data = []
    index = []
    for csv_num in range(len(ev1_csv)):
        index.append(f"N{csv_num + 1}")
        row = []
        for key in data:
            if key != "svnum":
                row.extend(sbg_rtk_average[csv_num][key])
                row.extend(sbg_rtk_1sigma_variance[csv_num][key])
                row.extend(sbg_rtk_3sigma_variance[csv_num][key])
        for key in data:
            if key != "svnum":
                row.extend(ev1_average[csv_num][key])
                row.extend(ev1_1signa_variance[csv_num][key])
                row.extend(ev1_3sigma_variance[csv_num][key])
        row.extend([ev1_ave_sat_num[csv_num], ev1_max_sat_num[csv_num], ev1_min_sat_num[csv_num]])
        ev1_data.append(row)

    fig, ax = plt.subplots()
    ax.axis("off")
    ax.table(cellText=ev1_data, colLabels=columns, rowLabels=index, loc="center")
    plt.savefig(output_dir / "ev1_latitude_longitude_average_and_variance.png", dpi=300, bbox_inches="tight")
    plt.close(fig)

    # 3σのうち緯度・経度の大きい方で、ばらつきが最大の測定回を選ぶ
    ev1_max_variance = -1
    ev1_max_csv_num = None
    for csv_num in range(len(ev1_csv)):
        if ev1_3sigma_variance[csv_num][lat_key] and ev1_3sigma_variance[csv_num][lon_key]:
            variance = max(ev1_3sigma_variance[csv_num][lat_key][0], ev1_3sigma_variance[csv_num][lon_key][0])
            if variance > ev1_max_variance:
                ev1_max_variance = variance
                ev1_max_csv_num = csv_num

    if ev1_max_csv_num is not None and ev1_max_csv_num < len(sbg_rtk_plot_lat):
        csv_num = ev1_max_csv_num
        fig, ax = plt.subplots()
        ax.scatter(sbg_rtk_plot_lat[csv_num], sbg_rtk_plot_lon[csv_num], label="SBG RTK reference", alpha=0.6)
        ax.scatter(ev1_plot_lat[csv_num], ev1_plot_lon[csv_num], label="EV-1 single positioning", alpha=0.6)
        ax.set_xlabel("Latitude")
        ax.set_ylabel("Longitude")
        ax.set_title(f"EV-1 and SBG RTK (N{csv_num + 1}, maximum 3σ)")
        ax.legend()
        ax.grid(True)
        plt.savefig(output_dir / "ev1_maximum_variance_scatter.png", dpi=300, bbox_inches="tight")
        plt.close(fig)
else:
    print("please input ev-1_csv_file")

### SBGの表作成 ###
if len(sbg_csv) > 0:
    sbg_data = []
    index = []
    for csv_num in range(len(sbg_csv)):
        index.append(f"N{csv_num + 1}")
        row = []
        for key in data:
            if key != "svnum":
                row.extend(sbg_rtk_average[csv_num][key])
                row.extend(sbg_rtk_1sigma_variance[csv_num][key])
                row.extend(sbg_rtk_3sigma_variance[csv_num][key])
        for key in data:
            if key != "svnum":
                row.extend(sbg_average[csv_num][key])
                row.extend(sbg_1sigma_variance[csv_num][key])
                row.extend(sbg_3sigma_variance[csv_num][key])
        row.extend([sbg_ave_sat_num[csv_num], sbg_max_sat_num[csv_num], sbg_min_sat_num[csv_num]])
        sbg_data.append(row)

    fig, ax = plt.subplots()
    ax.axis("off")
    ax.table(cellText=sbg_data, colLabels=columns, rowLabels=index, loc="center")
    plt.savefig(output_dir / "sbg_latitude_longitude_average_and_variance.png", dpi=300, bbox_inches="tight")
    plt.close(fig)

    # 3σのうち緯度・経度の大きい方で、ばらつきが最大の測定回を選ぶ
    sbg_max_variance = -1
    sbg_max_csv_num = None
    for csv_num in range(len(sbg_csv)):
        if sbg_3sigma_variance[csv_num][lat_key] and sbg_3sigma_variance[csv_num][lon_key]:
            variance = max(sbg_3sigma_variance[csv_num][lat_key][0], sbg_3sigma_variance[csv_num][lon_key][0])
            if variance > sbg_max_variance:
                sbg_max_variance = variance
                sbg_max_csv_num = csv_num

    if sbg_max_csv_num is not None and sbg_max_csv_num < len(sbg_rtk_plot_lat):
        csv_num = sbg_max_csv_num
        fig, ax = plt.subplots()
        ax.scatter(sbg_rtk_plot_lat[csv_num], sbg_rtk_plot_lon[csv_num], label="SBG RTK reference", alpha=0.6)
        ax.scatter(sbg_plot_lat[csv_num], sbg_plot_lon[csv_num], label="SBG single positioning", alpha=0.6)
        ax.set_xlabel("Latitude")
        ax.set_ylabel("Longitude")
        ax.set_title(f"SBG and SBG RTK (N{csv_num + 1}, maximum 3σ)")
        ax.legend()
        ax.grid(True)
        plt.savefig(output_dir / "sbg_maximum_variance_scatter.png", dpi=300, bbox_inches="tight")
        plt.close(fig)
else:
    print("please input sbg_csv_file")
