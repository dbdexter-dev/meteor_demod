# Meteor-M2 Demodulator

The goal of this project is to create a cross-platform QPSK demodulator for the
Meteor-M2 Russian weather satellite's LRPT downlink. It'll support reading from
file or directly from an attached rtl-sdr, and outputting the demodulated data
to a file or to a TCP socket, so that the image can be decoded in real-time.

# To-Do

## Critical
- Interpolator output has to be passed through a FIR filter to smooth it a bit
- The costas loop filters are still TBD
- Symbol timing loop recovery still TBD (Gardner? Mueller? Early-Late?)
- Write a GUI (GTK+) or a TUI (ncurses)

## Other

- Write a TCP server to interface with LRPTOfflineDecoder or similar
- Allow input directly from an rtl-sdr, and possibly other SDRs
- Allow parameter tweaking through command-line args
- Unit tests perhaps?

