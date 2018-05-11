# Meteor-M2 Demodulator

The goal of this project is to create a cross-platform QPSK demodulator for the
Meteor-M2 Russian weather satellite's LRPT downlink. It'll support reading from
file or directly from an attached rtl-sdr, and outputting the demodulated data
to a file or to a TCP socket, so that the image can be decoded in real-time.
