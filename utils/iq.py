#!/usr/bin/python

import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy.fft as fft
import sys

fig = plt.figure()
ax1 = fig.add_subplot(1,1,1)
globaldata = []
offset = 0
prog = 2000

def animate(i):
    global offset
    global prog

    data = globaldata[offset:offset+prog]
    offset += prog
    xar = []
    yar = []
    iar = []
    jar = []
    kar = []
    lar = []

    for point in data:
        if len(point) > 0 and "," in point:
            try:
                x,y,i,j,k,l = point.split(",")
            except ValueError:
                continue

            try:
                xar.append(float(x))
            except ValueError:
                xar.append(0)

            try:
                yar.append(float(y))
            except ValueError:
                yar.append(0)

            try:
                iar.append(float(i))
            except ValueError:
                iar.append(0)

            try:
                jar.append(float(j))
            except ValueError:
                jar.append(0)
            try:
                kar.append(float(k))
            except ValueError:
                kar.append(0)
            try:
                lar.append(float(l))
            except ValueError:
                lar.append(0)


    ax1.clear()
    ax1.set_ylim(-300,300)
    ax1.set_xlim(-300,300)
    ax1.scatter(xar, yar, color='C0')
    ax1.scatter(iar, jar, color='C1')
    ax1.scatter(kar, lar, color='C3')

try:
    globaldata = open("/tmp/iqdat").read().split()
except FileNotFoundError:
    sys.exit(0)
ani = animation.FuncAnimation(fig, animate, interval=500)
plt.show()

