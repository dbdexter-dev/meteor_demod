#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy.fft

a = open("/tmp/plot").read().split("\n")
a.pop()
a = a[-200000:]

a_fft = numpy.fft.rfft(a)
plt.plot(abs(a_fft))
plt.xlabel("Frequency")
plt.show()
