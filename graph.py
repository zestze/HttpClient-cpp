#!usr/bin/env python3

# Script for graphing results of using various threads
# in http client

import matplotlib.pyplot as plt

def graph():
    vals = []
    with open('results.csv') as f:
        for line in f:
            # should only be one line
            vals.append(list(map(float, line[:len(line) - 1].split(','))))

if __name__ == "__main__":
    graph()
