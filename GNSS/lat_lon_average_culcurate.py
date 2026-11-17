import json
import csv
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colormaps
from pathlib import Path

###EV1及びSBGのcsvファイルを読み込む###


###計算したいデータ項目を抽出する###
data = []
data_jsonfile = open("evaluate_key.json", "r")
data_jsondata = json.load(data_jsonfile)
for key in data_jsondata:
    data.append(data_jsondata[key])

for i in range(len(data)):
    print(data[i])

