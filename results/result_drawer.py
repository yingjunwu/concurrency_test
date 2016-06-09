from matplotlib.font_manager import FontProperties
import matplotlib.pyplot as plt
import matplotlib
import numpy as np
import pickle
import os
import re

import csv

OPT_FONT_NAME = 'Helvetica'
TICK_FONT_SIZE = 24
LABEL_FONT_SIZE = 26
LEGEND_FONT_SIZE = 24
EXP_LABEL_FONT_SIZE = 20
EXP_LEGEND_FONT_SIZE = 16.5
LABEL_FP = FontProperties(style='normal', size=LABEL_FONT_SIZE)
LEGEND_FP = FontProperties(style='normal', size=LEGEND_FONT_SIZE)
EXP_LABEL_FP = FontProperties(style='normal', size=EXP_LABEL_FONT_SIZE)
EXP_LEGEND_FP = FontProperties(style='normal', size=EXP_LEGEND_FONT_SIZE)
TICK_FP = FontProperties(style='normal', size=TICK_FONT_SIZE)

OPT_MARKERS = (['o', 's', 'v', "^", "h", "v", ">", "x", "d", "<", "|", "", "|", "_"])
COLOR_MAP = ('#F15854', '#5DA5DA', '#60BD68',  '#B276B2', '#DECF3F', '#F17CB0', '#B2912F', '#FAA43A', '#AFAFAF')
OPT_PATTERNS = ([ "////", "\\\\", "//////", "o", "o", "\\\\" , "\\\\" , "//////", "//////", "." , "\\\\\\" , "\\\\\\" ])
OPT_LABEL_WEIGHT = 'bold'
OPT_LINE_COLORS = COLOR_MAP
OPT_LINE_WIDTH = 4.0
OPT_MARKER_SIZE = 10.0

matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True
matplotlib.rcParams['xtick.labelsize'] = TICK_FONT_SIZE
matplotlib.rcParams['ytick.labelsize'] = TICK_FONT_SIZE
matplotlib.rcParams['font.family'] = OPT_FONT_NAME

homedir = "../figures/"


def ConvertEpsToPdf(dir_filename):
  os.system("epstopdf --outfile " + dir_filename + ".pdf " + dir_filename + ".eps")
  os.system("rm -rf " + dir_filename + ".eps")

def TransformData():
  configs = ['0,0,100','0,20,80','0,80,20','20,0,80','80,0,20','80,20,0','20,80,0']
  threads = [1,8,16,24,32,40]
  
  throughputs = np.zeros((len(configs), len(threads)))

  f = open('libcuckoo.log')

  while 1:
    line = f.readline().rstrip("\n")
    if not line:
      break

    match_res = re.match(r'.*thread count = (.*), read = (.*), write = (.*), insert = (.*), throughput = (.*) M ops', line)
    thread = match_res.group(1)
    read = match_res.group(2)
    write = match_res.group(3)
    insert = match_res.group(4)
    throughput = match_res.group(5)
    
    conf_str = read + ',' + write + ',' + insert

    config_idx = configs.index(conf_str)
    thread_idx = threads.index(int(thread))
    
    throughputs[config_idx][thread_idx] = float(throughput)
  
  f.close()

  return throughputs
  

def DrawScalability(throughputs):
  # configs = ['0,0,100','0,20,80','0,80,20','20,0,80','80,0,20','80,20,0','20,80,0']
  configs = ['0,0,100','0,20,80','0,80,20','20,0,80','80,0,20']
  threads = [1,8,16,24,32,40]

  YCSB_PROTO_LABEL = [ \
  'R=0, W=0, I=100', \
  'R=0, W=20, I=80', \
  'R=0, W=80, I=20', \
  'R=20, W=0, I=80', \
  'R=80, W=0, I=20', \
  # 'R=20, W=80, I=0', \
  # 'R=80, W=20, I=0', \
  ]
  
  matplotlib.rcParams['xtick.labelsize'] = 20
  matplotlib.rcParams['ytick.labelsize'] = 20
  
  fig = plt.figure(figsize=(8,3.2))
  figure = fig.add_subplot(111)
  
  for i in range(len(configs)):
      figure.plot(threads, throughputs[i], color=OPT_LINE_COLORS[i], \
                  linewidth=OPT_LINE_WIDTH, marker=OPT_MARKERS[i], \
                  markersize=OPT_MARKER_SIZE, label=YCSB_PROTO_LABEL[i])
  
  plt.xticks(threads)
  plt.xlim(1, 40)
  
  figure.get_xaxis().set_tick_params(direction='in', pad=10)
  figure.get_yaxis().set_tick_params(direction='in', pad=10)
  
  plt.xlabel('Number of threads', fontproperties=EXP_LABEL_FP)
  plt.ylabel('Throughput (M ops)', fontproperties=EXP_LABEL_FP)
  
  size = fig.get_size_inches()
  dpi = fig.get_dpi()
  plt.legend(bbox_to_anchor=(-0.15,1.5,1.2,0),loc=2,ncol=3,frameon=False, \
             prop=EXP_LEGEND_FP, labelspacing=0., mode="expand", \
             handletextpad=0.2, handleheight=2)
  
  # variable: core_cnt, proto
  filename = 'myfigure2'
  plt.savefig(filename + ".eps", bbox_inches='tight', format='eps')
  ConvertEpsToPdf(filename)


if __name__ == "__main__":
  throughputs = TransformData()
  DrawScalability(throughputs)