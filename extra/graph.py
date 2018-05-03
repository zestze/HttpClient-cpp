#!/usr/bin/env python3

# Script for graphing results of using various threads
# in http client

import matplotlib.pyplot as plt

def graph():
    vals = []
    with open('../log.txt') as f:
        for line in f:
            words = line[:len(line) - 1].split('\t')
            if words[0] != 'real':
                continue
            words = [word for word in words if word != '']
            vals.append(words[-1])

    y_vals = []
    for v in vals:
        temp = v[:len(v) - 1].split('m')
        y_vals.append(float(temp[0]) * 60 + float(temp[1]))


    y_vals_prime = []
    for n in range(len(y_vals) // 3):
        temp = 0
        for i in range(0, 3):
            temp += y_vals[3 * n + i]
        y_vals_prime.append(temp / 3)

    x_vals = range(0, 17)
    plt.plot(x_vals, y_vals_prime, 'bv--')
    plt.title('Number of Threads Used for Download against Download Time')
    plt.ylabel('Download Time (s)')
    plt.xlabel('Numer of Threads used')

    plt.show()

if __name__ == "__main__":
    graph()
