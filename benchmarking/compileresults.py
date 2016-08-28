#!/usr/bin/python3

import os
import re
import csv

f = open("/dev/null", "w")
writer = None

TEST_START = "TEST START: "
TEST_ITER = "TEST RESULTS: "
TEST_COMPLETE = "TEST COMPLETE"

os.makedirs("results", exist_ok=True)

fields = ("iteration", "nreps", "total duration", "duration")

while True:
    line = input()
    if line.startswith(TEST_START):
        f = open(os.path.join("results", line[len(TEST_START):] + ".csv"), "w")
        writer = csv.DictWriter(f, fields)
        writer.writeheader()
    elif line == TEST_COMPLETE:
        f.close()
    elif line.startswith(TEST_ITER) and writer is not None:
        iteration, nreps, total_duration = map(int, re.findall("\d+", line))
        duration = float(total_duration) / nreps
        writer.writerow(dict(zip(fields, (iteration, nreps, total_duration, duration))))
    else:
        print(line)

