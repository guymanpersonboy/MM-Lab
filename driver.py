#! /usr/bin/env python3
import subprocess
import statistics
import os
from tabulate import tabulate

def performance_check(trace_file):
    N = 20
    total_time = 0
    for i in range(0, N):
        performance = subprocess.run(["./performance", trace_file], universal_newlines=True, stdout=subprocess.PIPE)
        if 'Success' not in performance.stdout:
            return -1
        total_time += int(performance.stdout.split()[1])
    return total_time // N

def utilization_check(trace_file):
    utilization = subprocess.run(["./runner", '-ru', trace_file], universal_newlines=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if utilization.returncode != 0:
        return -1
    return_array = utilization.stdout.split('\n')
    utilization_percentage = float(return_array[4].split()[3])
    return utilization_percentage

def correctness_check(trace_file):
    correctness = subprocess.run(["./runner", '-r', trace_file], universal_newlines=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if correctness.returncode != 0:
        return False
    return 'umalloc package passed correctness check.' in correctness.stdout

trace_correctness = []
trace_utilization = []
trace_performance = []
table = []

def run_trace(trace_file):
    global trace_correctness
    global trace_utilization
    global trace_performance
    global table
    passed = correctness_check(trace_file)
    trace_correctness += [passed]
    util = -1
    perf = -1
    if passed:
        util = utilization_check(trace_file)
        perf = performance_check(trace_file)
    correct = 'Yes' if passed else 'No' 
    trace_utilization += [util]
    trace_performance += [perf]
    table += [[trace_file, correct, util, perf]]

#TODO: Calculate Score
os.system("make all")
for file in os.listdir("./traces"):
    if file.endswith(".rep"):
        run_trace(os.path.join("./traces", file))
print (tabulate(table, headers=["Trace", "Passed", "Utilization", "Performance (microseconds)"]))