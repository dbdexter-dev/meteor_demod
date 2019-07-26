#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy.fft

a = open("/tmp/plot").read().split("\n")
a.pop()

a_fft = numpy.fft.rfft(a)
plt.plot(a_fft)
plt.xlabel("Frequency")
plt.show()
